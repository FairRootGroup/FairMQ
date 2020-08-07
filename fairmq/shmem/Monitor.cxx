/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Monitor.h"
#include "Common.h"

#include <fairmq/Tools.h>
#include <fairlogger/Logger.h>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/file_mapping.hpp>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <csignal>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <time.h>
#include <iomanip>

#include <termios.h>
#include <poll.h>

using namespace std;
using bie = ::boost::interprocess::interprocess_exception;
namespace bipc = ::boost::interprocess;
namespace bpt = ::boost::posix_time;

namespace
{
    volatile sig_atomic_t gSignalStatus = 0;
}

namespace fair
{
namespace mq
{
namespace shmem
{

void signalHandler(int signal)
{
    gSignalStatus = signal;
}

Monitor::Monitor(const string& shmId, bool selfDestruct, bool interactive, bool viewOnly, unsigned int timeoutInMS, unsigned int intervalInMS, bool runAsDaemon, bool cleanOnExit)
    : fSelfDestruct(selfDestruct)
    , fInteractive(interactive)
    , fViewOnly(viewOnly)
    , fIsDaemon(runAsDaemon)
    , fSeenOnce(false)
    , fCleanOnExit(cleanOnExit)
    , fTimeoutInMS(timeoutInMS)
    , fIntervalInMS(intervalInMS)
    , fShmId(shmId)
    , fSegmentName("fmq_" + fShmId + "_main")
    , fManagementSegmentName("fmq_" + fShmId + "_mng")
    , fControlQueueName("fmq_" + fShmId + "_cq")
    , fTerminating(false)
    , fHeartbeatTriggered(false)
    , fLastHeartbeat(chrono::high_resolution_clock::now())
    , fSignalThread()
    , fDeviceHeartbeats()
{
    if (!fViewOnly) {
        try {
            bipc::named_mutex monitorStatus(bipc::create_only, string("fmq_" + fShmId + "_ms").c_str());
        } catch (bie&) {
            cout << "fairmq-shmmonitor for shared memory id " << fShmId << " already started or not properly exited. Try `fairmq-shmmonitor --cleanup --shmid " << fShmId << "`" << endl;
            throw DaemonPresent(tools::ToString("fairmq-shmmonitor for shared memory id ", fShmId, " already started or not properly exited."));
        }
    }

    Logger::SetConsoleColor(false);
    Logger::DefineVerbosity(Verbosity::user1, VerbositySpec::Make(VerbositySpec::Info::timestamp_us));
    Logger::SetVerbosity(Verbosity::verylow);
}

void Monitor::CatchSignals()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    fSignalThread = thread(&Monitor::SignalMonitor, this);
}

void Monitor::SignalMonitor()
{
    while (true) {
        if (gSignalStatus != 0) {
            fTerminating = true;
            cout << "signal: " << gSignalStatus << endl;
            break;
        } else if (fTerminating) {
            break;
        }

        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void Monitor::Run()
{
    thread heartbeatThread;
    if (!fViewOnly) {
        RemoveQueue(fControlQueueName);
        heartbeatThread = thread(&Monitor::MonitorHeartbeats, this);
    }

    if (fInteractive) {
        Interactive();
    } else {
        while (!fTerminating) {
            this_thread::sleep_for(chrono::milliseconds(fIntervalInMS));
            CheckSegment();
        }
    }

    if (!fViewOnly) {
        heartbeatThread.join();
    }
}

void Monitor::MonitorHeartbeats()
{
    try {
        bipc::message_queue mq(bipc::open_or_create, fControlQueueName.c_str(), 1000, 256);

        unsigned int priority;
        bipc::message_queue::size_type recvdSize;
        char msg[256] = {0};

        while (!fTerminating) {
            bpt::ptime rcvTill = bpt::microsec_clock::universal_time() + bpt::milliseconds(100);
            if (mq.timed_receive(&msg, sizeof(msg), recvdSize, priority, rcvTill)) {
                fHeartbeatTriggered = true;
                fLastHeartbeat = chrono::high_resolution_clock::now();
                string deviceId(msg, recvdSize);
                fDeviceHeartbeats[deviceId] = fLastHeartbeat;
            } else {
                // cout << "control queue timeout" << endl;
            }
        }
    } catch (bie& ie) {
        cout << ie.what() << endl;
    }

    RemoveQueue(fControlQueueName);
}

struct TerminalConfig
{
    TerminalConfig()
    {
        termios t;
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag &= ~ICANON; // disable canonical input
        t.c_lflag &= ~ECHO; // do not echo input chars
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings
    }

    ~TerminalConfig()
    {
        termios t;
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag |= ICANON; // re-enable canonical input
        t.c_lflag |= ECHO; // echo input chars
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings
    }
};

void Monitor::Interactive()
{
    char c;
    pollfd cinfd[1];
    cinfd[0].fd = fileno(stdin);
    cinfd[0].events = POLLIN;

    TerminalConfig tcfg;

    cout << endl;
    PrintHelp();
    cout << endl;
    PrintHeader();

    while (!fTerminating) {
        if (poll(cinfd, 1, fIntervalInMS)) {
            if (fTerminating || gSignalStatus != 0) {
                break;
            }

            c = getchar();

            switch (c) {
                case 'q':
                    cout << "\n[q] --> quitting." << endl;
                    fTerminating = true;
                    break;
                case 'p':
                    cout << "\n[p] --> active queues:" << endl;
                    PrintQueues();
                    break;
                case 'x':
                    cout << "\n[x] --> closing shared memory:" << endl;
                    if (!fViewOnly) {
                        Cleanup(ShmId{fShmId});
                    } else {
                        cout << "cannot close because in view only mode" << endl;
                    }
                    break;
                case 'h':
                    cout << "\n[h] --> help:" << endl << endl;
                    PrintHelp();
                    cout << endl;
                    break;
                case '\n':
                    cout << "\n[\\n] --> invalid input." << endl;
                    break;
                case 'b':
                    PrintDebug(ShmId{fShmId});
                    break;
                default:
                    cout << "\n[" << c << "] --> invalid input." << endl;
                    break;
            }

            if (fTerminating) {
                break;
            }

            PrintHeader();
        }

        if (fTerminating) {
            break;
        }

        CheckSegment();

        if (!fTerminating) {
            cout << "\r";
        }
    }
}

void Monitor::CheckSegment()
{
    char c = '#';

    if (fInteractive) {
        static uint64_t counter = 0;
        int mod = counter++ % 5;
        switch (mod) {
            case 0:
                c = '-';
                break;
            case 1:
                c = '\\';
                break;
            case 2:
                c = '|';
                break;
            case 3:
                c = '-';
                break;
            case 4:
                c = '/';
                break;
            default:
                break;
        }
    }

    try {
        bipc::managed_shared_memory segment(bipc::open_only, fSegmentName.c_str());
        bipc::managed_shared_memory managementSegment(bipc::open_only, fManagementSegmentName.c_str());

        fSeenOnce = true;

        unsigned int numDevices = 0;
        unsigned int numMessages = 0;

        if (fInteractive || fViewOnly) {
            DeviceCounter* dc = managementSegment.find<DeviceCounter>(bipc::unique_instance).first;
            if (dc) {
                numDevices = dc->fCount;
            }
            MsgCounter* mc = managementSegment.find<MsgCounter>(bipc::unique_instance).first;
            if (mc) {
                numMessages = mc->fCount;
            }
        }

        auto now = chrono::high_resolution_clock::now();
        unsigned int duration = chrono::duration_cast<chrono::milliseconds>(now - fLastHeartbeat).count();

        if (fHeartbeatTriggered && duration > fTimeoutInMS) {
            cout << "no heartbeats since over " << fTimeoutInMS << " milliseconds, cleaning..." << endl;
            Cleanup(ShmId{fShmId});
            fHeartbeatTriggered = false;
            if (fSelfDestruct) {
                cout << "\nself destructing" << endl;
                fTerminating = true;
            }
        }

        if (fInteractive) {
            cout << "| "
                 << setw(18) << fSegmentName                                    << " | "
                 << setw(10) << segment.get_size()                              << " | "
                 << setw(10) << segment.get_free_memory()                       << " | "
                 << setw(8)  << numDevices                                      << " | "
                 << setw(8)  << numMessages                                     << " | "
                 << setw(10) << (fViewOnly ? "view only" : to_string(duration)) << " |"
                 << c << flush;
        } else if (fViewOnly) {
            size_t free = segment.get_free_memory();
            size_t total = segment.get_size();
            size_t used = total - free;
            // size_t mfree = managementSegment.get_free_memory();
            // size_t mtotal = managementSegment.get_size();
            // size_t mused = mtotal - mfree;
            LOGV(info, user1) << "[" << fSegmentName
                              << "] devices: " << numDevices
                              << ", total: " << total
                              << ", msgs: " << numMessages
                              << ", free: " << free
                              << ", used: " << used;
                            //   << "\n                  "
                            //   << "[" << fManagementSegmentName
                            //   << "] total: " << mtotal
                            //   << ", free: " << mfree
                            //   << ", used: " << mused;
        }
    } catch (bie&) {
        fHeartbeatTriggered = false;
        if (fInteractive) {
            cout << "| "
                 << setw(18) << "-" << " | "
                 << setw(10) << "-" << " | "
                 << setw(10) << "-" << " | "
                 << setw(8)  << "-" << " | "
                 << setw(8)  << "-" << " | "
                 << setw(10) << "-" << " |"
                 << c << flush;
        }

        auto now = chrono::high_resolution_clock::now();
        unsigned int duration = chrono::duration_cast<chrono::milliseconds>(now - fLastHeartbeat).count();

        if (fIsDaemon && duration > fTimeoutInMS * 2) {
            Cleanup(ShmId{fShmId});
            fHeartbeatTriggered = false;
            if (fSelfDestruct) {
                cout << "\nself destructing" << endl;
                fTerminating = true;
            }
        }

        if (fSelfDestruct) {
            if (fSeenOnce) {
                cout << "self destructing" << endl;
                fTerminating = true;
            }
        }
    }
}

void Monitor::PrintDebug(const ShmId& shmId)
{
    string managementSegmentName("fmq_" + shmId.shmId + "_mng");
    try {
        bipc::managed_shared_memory managementSegment(bipc::open_only, managementSegmentName.c_str());
        boost::interprocess::named_mutex mtx(boost::interprocess::open_only, std::string("fmq_" + shmId.shmId + "_mtx").c_str());
        boost::interprocess::scoped_lock<bipc::named_mutex> lock(mtx);

        Uint64MsgDebugMap* debug = managementSegment.find<Uint64MsgDebugMap>(bipc::unique_instance).first;

        cout << endl << "found " << debug->size() << " message(s):" << endl;

        for (const auto& e : *debug) {
            using time_point = std::chrono::system_clock::time_point;
            time_point tmpt{std::chrono::duration_cast<time_point::duration>(std::chrono::nanoseconds(e.second.fCreationTime))};
            std::time_t t = std::chrono::system_clock::to_time_t(tmpt);
            uint64_t ms = e.second.fCreationTime % 1000000;
            auto tm = localtime(&t);
            cout << "offset: " << setw(12) << setfill(' ') << e.first
                 << ", size: " << setw(10) << setfill(' ') << e.second.fSize
                 << ", creator PID: " << e.second.fPid << setfill('0')
                 << ", at: " << setw(2) << tm->tm_hour << ":" << setw(2) << tm->tm_min << ":" << setw(2) << tm->tm_sec << "." << setw(6) << ms << endl;
        }
        cout << setfill(' ');
    } catch (bie&) {
        cout << "no segment found" << endl;
    }
}

void Monitor::PrintQueues()
{
    cout << '\n';

    try {
        bipc::managed_shared_memory segment(bipc::open_only, fSegmentName.c_str());
        StrVector* queues = segment.find<StrVector>(string("fmq_" + fShmId + "_qs").c_str()).first;
        if (queues) {
            cout << "found " << queues->size() << " queue(s):" << endl;

            for (const auto& queue : *queues) {
                string name(queue.c_str());
                cout << '\t' << name << " : ";
                atomic<int>* queueSize = segment.find<atomic<int>>(name.c_str()).first;
                if (queueSize) {
                    cout << *queueSize << " messages" << endl;
                } else {
                    cout << "\tqueue does not have a queue size entry." << endl;
                }
            }
        } else {
            cout << "\tno queues found" << endl;
        }
    } catch (bie&) {
        cout << "\tno queues found" << endl;
    } catch (out_of_range&) {
        cout << "\tno queues found" << endl;
    }

    cout << "\n    --> last heartbeats: " << endl << endl;
    auto now = chrono::high_resolution_clock::now();
    for (const auto& h : fDeviceHeartbeats)  {
        cout << "\t" << h.first << " : " << chrono::duration<double, milli>(now - h.second).count() << "ms ago." << endl;
    }

    cout << endl;
}

void Monitor::PrintHeader()
{
    cout << "| "
         << setw(18) << "name"    << " | "
         << setw(10) << "size"    << " | "
         << setw(10) << "free"    << " | "
         << setw(8)  << "devices" << " | "
         << setw(8)  << "msgs"    << " | "
         << setw(10) << "last hb" << " |"
         << endl;
}

void Monitor::PrintHelp()
{
    cout << "controls: [x] close memory, [p] print queues, [] print a list of allocated messages, [h] help, [q] quit." << endl;
}

void Monitor::RemoveObject(const string& name)
{
    if (bipc::shared_memory_object::remove(name.c_str())) {
        cout << "Successfully removed '" << name << "'." << endl;
    } else {
        cout << "Did not remove '" << name << "'. Already removed?" << endl;
    }
}

void Monitor::RemoveFileMapping(const string& name)
{
    if (bipc::file_mapping::remove(name.c_str())) {
        cout << "Successfully removed '" << name << "'." << endl;
    } else {
        cout << "Did not remove '" << name << "'. Already removed?" << endl;
    }
}

void Monitor::RemoveQueue(const string& name)
{
    if (bipc::message_queue::remove(name.c_str())) {
        cout << "Successfully removed '" << name << "'." << endl;
    } else {
        cout << "Did not remove '" << name << "'. Already removed?" << endl;
    }
}

void Monitor::RemoveMutex(const string& name)
{
    if (bipc::named_mutex::remove(name.c_str())) {
        cout << "Successfully removed '" << name << "'." << endl;
    } else {
        cout << "Did not remove '" << name << "'. Already removed?" << endl;
    }
}

void Monitor::RemoveCondition(const string& name)
{
    if (bipc::named_condition::remove(name.c_str())) {
        cout << "Successfully removed '" << name << "'." << endl;
    } else {
        cout << "Did not remove '" << name << "'. Already removed?" << endl;
    }
}

void Monitor::Cleanup(const ShmId& shmId)
{
    cout << "Cleaning up for shared memory id '" << shmId.shmId << "'..." << endl;
    string managementSegmentName("fmq_" + shmId.shmId + "_mng");
    try {
        bipc::managed_shared_memory managementSegment(bipc::open_only, managementSegmentName.c_str());
        RegionCounter* rc = managementSegment.find<RegionCounter>(bipc::unique_instance).first;
        if (rc) {
            cout << "Region counter found: " << rc->fCount << endl;
            uint64_t regionCount = rc->fCount;

            Uint64RegionInfoMap* m = managementSegment.find<Uint64RegionInfoMap>(bipc::unique_instance).first;

            for (uint64_t i = 1; i <= regionCount; ++i) {
                if (m != nullptr) {
                    RegionInfo ri = m->at(i);
                    string path = ri.fPath.c_str();
                    int flags = ri.fFlags;
                    cout << "Found RegionInfo with path: '" << path << "', flags: " << flags << ", fDestroyed: " << ri.fDestroyed << "." << endl;
                    if (path != "") {
                        RemoveFileMapping(tools::ToString(path, "fmq_" + shmId.shmId + "_rg_" + to_string(i)));
                    } else {
                        RemoveObject("fmq_" + shmId.shmId + "_rg_" + to_string(i));
                    }
                } else {
                    RemoveObject("fmq_" + shmId.shmId + "_rg_" + to_string(i));
                }

                RemoveQueue(string("fmq_" + shmId.shmId + "_rgq_" + to_string(i)));
            }
        } else {
            cout << "No region counter found. No regions to cleanup." << endl;
        }

        RemoveObject(managementSegmentName.c_str());
    } catch (bie&) {
        cout << "Did not find '" << managementSegmentName << "' shared memory segment. No regions to cleanup." << endl;
    } catch(out_of_range& oor) {
        cout << "Could not locate element in the region map, out of range: " << oor.what() << endl;
    }

    RemoveObject("fmq_" + shmId.shmId + "_main");
    RemoveMutex("fmq_" + shmId.shmId + "_mtx");
    RemoveCondition("fmq_" + shmId.shmId + "_cv");

    cout << endl;
}

void Monitor::Cleanup(const SessionId& sessionId)
{
    ShmId shmId{buildShmIdFromSessionIdAndUserId(sessionId.sessionId)};
    cout << "Cleanup called with session id '" << sessionId.sessionId << "', translating to shared memory id '" << shmId.shmId << "'" << endl;
    Cleanup(shmId);
}

void Monitor::CleanupFull(const ShmId& shmId)
{
    Cleanup(shmId);
    RemoveMutex("fmq_" + shmId.shmId + "_ms");
    RemoveQueue("fmq_" + shmId.shmId + "_cq");
}

void Monitor::CleanupFull(const SessionId& sessionId)
{
    ShmId shmId{buildShmIdFromSessionIdAndUserId(sessionId.sessionId)};
    cout << "Cleanup called with session id '" << sessionId.sessionId << "', translating to shared memory id '" << shmId.shmId << "'" << endl;
    CleanupFull(shmId);
}

Monitor::~Monitor()
{
    if (fSignalThread.joinable()) {
        fSignalThread.join();
    }
    if (fCleanOnExit) {
        Cleanup(ShmId{fShmId});
    }
    if (!fViewOnly) {
        RemoveMutex("fmq_" + fShmId + "_ms");
    }
}

} // namespace shmem
} // namespace mq
} // namespace fair

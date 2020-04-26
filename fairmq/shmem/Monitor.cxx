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

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/file_mapping.hpp>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <csignal>
#include <iostream>
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

Monitor::Monitor(const string& shmId, bool selfDestruct, bool interactive, bool viewOnly, unsigned int timeoutInMS, bool runAsDaemon, bool cleanOnExit)
    : fSelfDestruct(selfDestruct)
    , fInteractive(interactive)
    , fViewOnly(viewOnly)
    , fIsDaemon(runAsDaemon)
    , fSeenOnce(false)
    , fCleanOnExit(cleanOnExit)
    , fTimeoutInMS(timeoutInMS)
    , fShmId(shmId)
    , fSegmentName("fmq_" + fShmId + "_main")
    , fManagementSegmentName("fmq_" + fShmId + "_mng")
    , fControlQueueName("fmq_" + fShmId + "_cq")
    , fTerminating(false)
    , fHeartbeatTriggered(false)
    , fLastHeartbeat(chrono::high_resolution_clock::now())
    , fSignalThread()
    , fManagementSegment(bipc::open_or_create, fManagementSegmentName.c_str(), 65536)
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
            this_thread::sleep_for(chrono::milliseconds(100));
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
        if (poll(cinfd, 1, 100)) {
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
                        Cleanup(fShmId);
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

        if (fInteractive) {
            DeviceCounter* dc = managementSegment.find<DeviceCounter>(bipc::unique_instance).first;
            if (dc) {
                numDevices = dc->fCount;
            }
        }

        auto now = chrono::high_resolution_clock::now();
        unsigned int duration = chrono::duration_cast<chrono::milliseconds>(now - fLastHeartbeat).count();

        if (fHeartbeatTriggered && duration > fTimeoutInMS) {
            cout << "no heartbeats since over " << fTimeoutInMS << " milliseconds, cleaning..." << endl;
            Cleanup(fShmId);
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
                 << setw(10) << (fViewOnly ? "view only" : to_string(duration)) << " |"
                 << c << flush;
        }
    } catch (bie&) {
        fHeartbeatTriggered = false;
        if (fInteractive) {
            cout << "| "
                 << setw(18) << "-" << " | "
                 << setw(10) << "-" << " | "
                 << setw(10) << "-" << " | "
                 << setw(8)  << "-" << " | "
                 << setw(10) << "-" << " |"
                 << c << flush;
        }

        auto now = chrono::high_resolution_clock::now();
        unsigned int duration = chrono::duration_cast<chrono::milliseconds>(now - fLastHeartbeat).count();

        if (fIsDaemon && duration > fTimeoutInMS * 2) {
            Cleanup(fShmId);
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
         << setw(10) << "last hb" << " |"
         << endl;
}

void Monitor::PrintHelp()
{
    cout << "controls: [x] close memory, [p] print queues, [h] help, [q] quit." << endl;
}

void Monitor::RemoveObject(const string& name)
{
    if (bipc::shared_memory_object::remove(name.c_str())) {
        cout << "Successfully removed \"" << name << "\"." << endl;
    } else {
        cout << "Did not remove \"" << name << "\". Already removed?" << endl;
    }
}

void Monitor::RemoveFileMapping(const string& name)
{
    if (bipc::file_mapping::remove(name.c_str())) {
        cout << "Successfully removed \"" << name << "\"." << endl;
    } else {
        cout << "Did not remove \"" << name << "\". Already removed?" << endl;
    }
}

void Monitor::RemoveQueue(const string& name)
{
    if (bipc::message_queue::remove(name.c_str())) {
        cout << "Successfully removed \"" << name << "\"." << endl;
    } else {
        cout << "Did not remove \"" << name << "\". Already removed?" << endl;
    }
}

void Monitor::RemoveMutex(const string& name)
{
    if (bipc::named_mutex::remove(name.c_str())) {
        cout << "Successfully removed \"" << name << "\"." << endl;
    } else {
        cout << "Did not remove \"" << name << "\". Already removed?" << endl;
    }
}

void Monitor::RemoveCondition(const string& name)
{
    if (bipc::named_condition::remove(name.c_str())) {
        cout << "Successfully removed \"" << name << "\"." << endl;
    } else {
        cout << "Did not remove \"" << name << "\". Already removed?" << endl;
    }
}

void Monitor::Cleanup(const string& shmId)
{
    string managementSegmentName("fmq_" + shmId + "_mng");
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
                    cout << "Found RegionInfo with path: '" << path << "', flags: " << flags << "'." << endl;
                    if (path != "") {
                        RemoveFileMapping(tools::ToString(path, "fmq_" + shmId + "_rg_" + to_string(i)));
                    } else {
                        RemoveObject("fmq_" + shmId + "_rg_" + to_string(i));
                    }
                } else {
                    RemoveObject("fmq_" + shmId + "_rg_" + to_string(i));
                }

                RemoveQueue(string("fmq_" + shmId + "_rgq_" + to_string(i)));
            }
        } else {
            cout << "No region counter found. no regions to cleanup." << endl;
        }

        RemoveObject(managementSegmentName.c_str());
    } catch (bie&) {
        cout << "Did not find '" << managementSegmentName << "' shared memory segment. No regions to cleanup." << endl;
    } catch(std::out_of_range& oor) {
        cout << "Could not locate element in the region map, out of range: " << oor.what() << endl;
    }

    RemoveObject("fmq_" + shmId + "_main");
    RemoveMutex("fmq_" + shmId + "_mtx");
    RemoveCondition("fmq_" + shmId + "_cv");

    cout << endl;
}

Monitor::~Monitor()
{
    if (fSignalThread.joinable()) {
        fSignalThread.join();
    }
    if (fCleanOnExit) {
        Cleanup(fShmId);
    }
    if (!fViewOnly) {
        RemoveMutex("fmq_" + fShmId + "_ms");
    }
}

} // namespace shmem
} // namespace mq
} // namespace fair

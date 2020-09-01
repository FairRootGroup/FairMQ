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
#include <sstream>

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
    , fSegmentName("fmq_" + fShmId + "_m_0")
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
                    PrintDebugInfo(ShmId{fShmId});
                    break;
                default:
                    cout << "\n[" << c << "] --> invalid input." << endl;
                    break;
            }

            if (fTerminating) {
                break;
            }
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
    using namespace boost::interprocess;

    try {
        managed_shared_memory managementSegment(open_only, fManagementSegmentName.c_str());
        VoidAlloc allocInstance(managementSegment.get_segment_manager());

        Uint16SegmentInfoHashMap* segmentInfos = managementSegment.find<Uint16SegmentInfoHashMap>(unique_instance).first;
        std::unordered_map<uint16_t, boost::variant<RBTreeBestFitSegment, SimpleSeqFitSegment>> segments;

        if (!segmentInfos) {
            cout << "Found management segment, but cannot locate segment info, something went wrong..." << endl;
            return;
        }

        for (const auto& s : *segmentInfos) {
            if (s.second.fAllocationAlgorithm == AllocationAlgorithm::rbtree_best_fit) {
                segments.emplace(s.first, RBTreeBestFitSegment(open_only, std::string("fmq_" + fShmId + "_m_" + to_string(s.first)).c_str()));
            } else {
                segments.emplace(s.first, SimpleSeqFitSegment(open_only, std::string("fmq_" + fShmId + "_m_" + to_string(s.first)).c_str()));
            }
        }

        fSeenOnce = true;

        unsigned int numDevices = 0;
#ifdef FAIRMQ_DEBUG_MODE
        Uint16MsgCounterHashMap* msgCounters = nullptr;
#endif

        if (fInteractive || fViewOnly) {
            DeviceCounter* dc = managementSegment.find<DeviceCounter>(unique_instance).first;
            if (dc) {
                numDevices = dc->fCount;
            }
#ifdef FAIRMQ_DEBUG_MODE
            msgCounters = managementSegment.find_or_construct<Uint16MsgCounterHashMap>(unique_instance)(allocInstance);
#endif
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

        if (fInteractive || fViewOnly) {
            stringstream ss;
            size_t mfree = managementSegment.get_free_memory();
            size_t mtotal = managementSegment.get_size();
            size_t mused = mtotal - mfree;

            ss << "shm id: " << fShmId
               << ", devices: " << numDevices << ", segments:\n";
            for (const auto& s : segments) {
                size_t free = boost::apply_visitor(SegmentFreeMemory{}, s.second);
                size_t total = boost::apply_visitor(SegmentSize{}, s.second);
                size_t used = total - free;
                ss << "   [" << s.first
                   << "]: total: " << total
#ifdef FAIRMQ_DEBUG_MODE
                   << ", msgs: " << (*msgCounters)[s.first].fCount
#else
                   << ", msgs: NODEBUG"
#endif
                   << ", free: " << free
                   << ", used: " << used
                   << "\n";
            }
            ss << "   [m]: "
               << "total: " << mtotal
               << ", free: " << mfree
               << ", used: " << mused;
            LOGV(info, user1) << ss.str();
        }
    } catch (bie&) {
        fHeartbeatTriggered = false;

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

void Monitor::PrintDebugInfo(const ShmId& shmId __attribute__((unused)))
{
#ifdef FAIRMQ_DEBUG_MODE
    string managementSegmentName("fmq_" + shmId.shmId + "_mng");
    try {
        bipc::managed_shared_memory managementSegment(bipc::open_only, managementSegmentName.c_str());
        boost::interprocess::named_mutex mtx(boost::interprocess::open_only, string("fmq_" + shmId.shmId + "_mtx").c_str());
        boost::interprocess::scoped_lock<bipc::named_mutex> lock(mtx);

        Uint16MsgDebugMapHashMap* debug = managementSegment.find<Uint16MsgDebugMapHashMap>(bipc::unique_instance).first;

        size_t numMessages = 0;

        for (const auto& e : *debug) {
            numMessages += e.second.size();
        }
        cout << endl << "found " << numMessages << " messages." << endl;

        for (const auto& s : *debug) {
            for (const auto& e : s.second) {
                using time_point = chrono::system_clock::time_point;
                time_point tmpt{chrono::duration_cast<time_point::duration>(chrono::nanoseconds(e.second.fCreationTime))};
                time_t t = chrono::system_clock::to_time_t(tmpt);
                uint64_t ms = e.second.fCreationTime % 1000000;
                auto tm = localtime(&t);
                cout << "segment: " << setw(3) << setfill(' ') << s.first
                     << ", offset: " << setw(12) << setfill(' ') << e.first
                     << ", size: " << setw(10) << setfill(' ') << e.second.fSize
                     << ", creator PID: " << e.second.fPid << setfill('0')
                     << ", at: " << setw(2) << tm->tm_hour << ":" << setw(2) << tm->tm_min << ":" << setw(2) << tm->tm_sec << "." << setw(6) << ms << endl;
            }
        }
        cout << setfill(' ');
    } catch (bie&) {
        cout << "no segments found" << endl;
    }
#else
    cout << "FairMQ was not compiled in debug mode (FAIRMQ_DEBUG_MODE)" << endl;
#endif
}

void Monitor::PrintDebugInfo(const SessionId& sessionId)
{
    ShmId shmId{buildShmIdFromSessionIdAndUserId(sessionId.sessionId)};
    PrintDebugInfo(shmId);
}

unordered_map<uint16_t, std::vector<BufferDebugInfo>> Monitor::GetDebugInfo(const ShmId& shmId __attribute__((unused)))
{
    unordered_map<uint16_t, std::vector<BufferDebugInfo>> result;

#ifdef FAIRMQ_DEBUG_MODE
    string managementSegmentName("fmq_" + shmId.shmId + "_mng");
    try {
        bipc::managed_shared_memory managementSegment(bipc::open_only, managementSegmentName.c_str());
        boost::interprocess::named_mutex mtx(boost::interprocess::open_only, string("fmq_" + shmId.shmId + "_mtx").c_str());
        boost::interprocess::scoped_lock<bipc::named_mutex> lock(mtx);

        Uint16MsgDebugMapHashMap* debug = managementSegment.find<Uint16MsgDebugMapHashMap>(bipc::unique_instance).first;

        result.reserve(debug->size());

        for (const auto& s : *debug) {
            result[s.first].reserve(s.second.size());
            for (const auto& e : s.second) {
                result[s.first][e.first] = BufferDebugInfo(e.first, e.second.fPid, e.second.fSize, e.second.fCreationTime);
            }
        }
    } catch (bie&) {
        cout << "no segments found" << endl;
    }
#else
    cout << "FairMQ was not compiled in debug mode (FAIRMQ_DEBUG_MODE)" << endl;
#endif

    return result;
}
unordered_map<uint16_t, std::vector<BufferDebugInfo>> Monitor::GetDebugInfo(const SessionId& sessionId)
{
    ShmId shmId{buildShmIdFromSessionIdAndUserId(sessionId.sessionId)};
    return GetDebugInfo(shmId);
}

void Monitor::PrintHelp()
{
    cout << "controls: [x] close memory, "
         << "[b] print a list of allocated messages (only available when compiled with FAIMQ_DEBUG_MODE=ON), "
         << "[h] help, "
         << "[q] quit." << endl;
}


std::pair<std::string, bool> RunRemoval(std::function<bool(const std::string&)> f, std::string name, bool verbose)
{
    if (f(name)) {
        if (verbose) {
            cout << "Successfully removed '" << name << "'." << endl;
        }
        return {name, true};
    } else {
        if (verbose) {
            cout << "Did not remove '" << name << "'. Already removed?" << endl;
        }
        return {name, false};
    }
}

bool Monitor::RemoveObject(const string& name)      { return bipc::shared_memory_object::remove(name.c_str()); }
bool Monitor::RemoveFileMapping(const string& name) { return bipc::file_mapping::remove(name.c_str()); }
bool Monitor::RemoveQueue(const string& name)       { return bipc::message_queue::remove(name.c_str()); }
bool Monitor::RemoveMutex(const string& name)       { return bipc::named_mutex::remove(name.c_str()); }
bool Monitor::RemoveCondition(const string& name)   { return bipc::named_condition::remove(name.c_str()); }

std::vector<std::pair<std::string, bool>> Monitor::Cleanup(const ShmId& shmId, bool verbose /* = true */)
{
    std::vector<std::pair<std::string, bool>> result;

    if (verbose) {
        cout << "Cleaning up for shared memory id '" << shmId.shmId << "'..." << endl;
    }

    string managementSegmentName("fmq_" + shmId.shmId + "_mng");
    try {
        bipc::managed_shared_memory managementSegment(bipc::open_only, managementSegmentName.c_str());

        try {
            RegionCounter* rc = managementSegment.find<RegionCounter>(bipc::unique_instance).first;
            if (rc) {
                if (verbose) {
                    cout << "Region counter found: " << rc->fCount << endl;
                }
                uint16_t regionCount = rc->fCount;

                Uint16RegionInfoMap* m = managementSegment.find<Uint16RegionInfoMap>(bipc::unique_instance).first;

                for (uint16_t i = 1; i <= regionCount; ++i) {
                    if (m != nullptr) {
                        RegionInfo ri = m->at(i);
                        string path = ri.fPath.c_str();
                        int flags = ri.fFlags;
                        if (verbose) {
                            cout << "Found RegionInfo with path: '" << path << "', flags: " << flags << ", fDestroyed: " << ri.fDestroyed << "." << endl;
                        }
                        if (path != "") {
                            result.emplace_back(RunRemoval(Monitor::RemoveFileMapping, path + "fmq_" + shmId.shmId + "_rg_" + to_string(i), verbose));
                        } else {
                            result.emplace_back(RunRemoval(Monitor::RemoveObject, "fmq_" + shmId.shmId + "_rg_" + to_string(i), verbose));
                        }
                    } else {
                        result.emplace_back(RunRemoval(Monitor::RemoveObject, "fmq_" + shmId.shmId + "_rg_" + to_string(i), verbose));
                    }

                    result.emplace_back(RunRemoval(Monitor::RemoveQueue, string("fmq_" + shmId.shmId + "_rgq_" + to_string(i)), verbose));
                }
            } else {
                if (verbose) {
                    cout << "No region counter found. No regions to cleanup." << endl;
                }
            }
        } catch(out_of_range& oor) {
            if (verbose) {
                cout << "Could not locate element in the region map, out of range: " << oor.what() << endl;
            }
        }

        Uint16SegmentInfoHashMap* segmentInfos = managementSegment.find<Uint16SegmentInfoHashMap>(bipc::unique_instance).first;

        for (const auto& s : *segmentInfos) {
            result.emplace_back(RunRemoval(Monitor::RemoveObject, "fmq_" + shmId.shmId + "_m_" + to_string(s.first), verbose));
        }

        result.emplace_back(RunRemoval(Monitor::RemoveObject, managementSegmentName.c_str(), verbose));
    } catch (bie&) {
        if (verbose) {
            cout << "Did not find '" << managementSegmentName << "' shared memory segment. No regions to cleanup." << endl;
        }
    }

    result.emplace_back(RunRemoval(Monitor::RemoveMutex, "fmq_" + shmId.shmId + "_mtx", verbose));
    result.emplace_back(RunRemoval(Monitor::RemoveCondition, "fmq_" + shmId.shmId + "_cv", verbose));

    return result;
}

std::vector<std::pair<std::string, bool>> Monitor::Cleanup(const SessionId& sessionId, bool verbose /* = true */)
{
    ShmId shmId{buildShmIdFromSessionIdAndUserId(sessionId.sessionId)};
    if (verbose) {
        cout << "Cleanup called with session id '" << sessionId.sessionId << "', translating to shared memory id '" << shmId.shmId << "'" << endl;
    }
    return Cleanup(shmId, verbose);
}

std::vector<std::pair<std::string, bool>> Monitor::CleanupFull(const ShmId& shmId, bool verbose /* = true */)
{
    auto result = Cleanup(shmId, verbose);
    result.emplace_back(RunRemoval(Monitor::RemoveMutex, "fmq_" + shmId.shmId + "_ms", verbose));
    result.emplace_back(RunRemoval(Monitor::RemoveQueue, "fmq_" + shmId.shmId + "_cq", verbose));
    return result;
}

std::vector<std::pair<std::string, bool>> Monitor::CleanupFull(const SessionId& sessionId, bool verbose /* = true */)
{
    ShmId shmId{buildShmIdFromSessionIdAndUserId(sessionId.sessionId)};
    if (verbose) {
        cout << "Cleanup called with session id '" << sessionId.sessionId << "', translating to shared memory id '" << shmId.shmId << "'" << endl;
    }
    return CleanupFull(shmId, verbose);
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

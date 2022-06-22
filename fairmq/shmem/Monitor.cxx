/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Common.h"
#include "Monitor.h"
#include "Segment.h"
#include <fairmq/shmem/UnmanagedRegion.h>

#include <fairmq/tools/IO.h>
#include <fairmq/tools/Strings.h>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/file_mapping.hpp>

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <csignal>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <poll.h>

#if FAIRMQ_HAS_STD_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <boost/filesystem.hpp>
namespace fs = ::boost::filesystem;
#endif

using namespace std;
using bie = ::boost::interprocess::interprocess_exception;
namespace bipc = ::boost::interprocess;

namespace
{
    volatile sig_atomic_t gSignalStatus = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
}

namespace fair::mq::shmem
{

void signalHandler(int signal)
{
    gSignalStatus = signal;
}

Monitor::Monitor(string shmId, bool selfDestruct, bool interactive, bool viewOnly, unsigned int timeoutInMS, unsigned int intervalInMS, bool monitor, bool cleanOnExit)
    : fSelfDestruct(selfDestruct)
    , fInteractive(interactive)
    , fViewOnly(viewOnly)
    , fMonitor(monitor)
    , fSeenOnce(false)
    , fCleanOnExit(cleanOnExit)
    , fTimeoutInMS(timeoutInMS)
    , fIntervalInMS(intervalInMS)
    , fShmId(std::move(shmId))
    , fTerminating(false)
    , fHeartbeatTriggered(false)
    , fLastHeartbeat(chrono::high_resolution_clock::now())
{
    if (!fViewOnly) {
        try {
            bipc::named_mutex monitorStatus(bipc::create_only, string("fmq_" + fShmId + "_ms").c_str());
        } catch (bie&) {
            if (fInteractive) {
                LOG(error) << "fairmq-shmmonitor for shm id " << fShmId << " is already running. Try `fairmq-shmmonitor --cleanup --shmid " << fShmId << "`, or run in view-only mode (-v)";
            } else {
                LOG(error) << "fairmq-shmmonitor for shm id " << fShmId << " is already running. Try `fairmq-shmmonitor --cleanup --shmid " << fShmId << "`";
            }
            throw DaemonPresent(tools::ToString("fairmq-shmmonitor (in monitoring mode) for shm id ", fShmId, " is already running."));
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
            LOG(info) << "signal: " << gSignalStatus;
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
        heartbeatThread = thread(&Monitor::CheckHeartbeats, this);
    }

    if (fInteractive) {
        Interactive();
    } else {
        Watch();
    }

    if (!fViewOnly) {
        heartbeatThread.join();
    }
}

void Monitor::Watch()
{
    while (!fTerminating) {
        using namespace boost::interprocess;

        try {
            managed_shared_memory managementSegment(open_read_only, std::string("fmq_" + fShmId + "_mng").c_str());

            fSeenOnce = true;

            auto now = chrono::high_resolution_clock::now();
            unsigned int duration = chrono::duration_cast<chrono::milliseconds>(now - fLastHeartbeat.load()).count();

            if (fHeartbeatTriggered && duration > fTimeoutInMS) {
                // memory is present, but no heartbeats since timeout duration
                LOG(info) << "no heartbeats since over " << fTimeoutInMS << " milliseconds, cleaning...";
                Cleanup(ShmId{fShmId});
                fHeartbeatTriggered = false;
                if (fSelfDestruct) {
                    LOG(info) << "self destructing (segment has been observed and cleaned up by the monitor)";
                    fTerminating = true;
                }
            }
        } catch (bie&) {
            fHeartbeatTriggered = false;

            if (fSelfDestruct) {
                if (fSeenOnce) {
                    // segment has been observed at least once, can self-destruct
                    LOG(info) << "self destructing (segment has been observed and cleaned up orderly)";
                    fTerminating = true;
                } else {
                    // if self-destruct is requested, and no segment has ever been observed, quit after double timeout duration
                    auto now = chrono::high_resolution_clock::now();
                    unsigned int duration = chrono::duration_cast<chrono::milliseconds>(now - fLastHeartbeat.load()).count();

                    if (duration > fTimeoutInMS * 2) {
                        Cleanup(ShmId{fShmId});
                        LOG(info) << "self destructing (no segments observed within (timeout * 2) since start)";
                        fTerminating = true;
                    }
                }
            }
        }

        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

bool Monitor::PrintShm(const ShmId& shmId)
{
    using namespace boost::interprocess;

    try {
        managed_shared_memory managementSegment(open_read_only, std::string("fmq_" + shmId.shmId + "_mng").c_str());
        VoidAlloc allocInstance(managementSegment.get_segment_manager());

        Uint16SegmentInfoHashMap* shmSegments = managementSegment.find<Uint16SegmentInfoHashMap>(unique_instance).first;
        std::unordered_map<uint16_t, boost::variant<RBTreeBestFitSegment, SimpleSeqFitSegment>> segments;

        Uint16RegionInfoHashMap* shmRegions = managementSegment.find<Uint16RegionInfoHashMap>(unique_instance).first;

        if (!shmSegments) {
            LOG(error) << "Found management segment, but cannot locate segment info, something went wrong...";
            return false;
        }

        if (!shmRegions) {
            LOG(error) << "Found management segment, but cannot locate region info...";
        }

        for (const auto& s : *shmSegments) {
            if (s.second.fAllocationAlgorithm == AllocationAlgorithm::rbtree_best_fit) {
                segments.emplace(s.first, RBTreeBestFitSegment(open_read_only, std::string("fmq_" + shmId.shmId + "_m_" + to_string(s.first)).c_str()));
            } else {
                segments.emplace(s.first, SimpleSeqFitSegment(open_read_only, std::string("fmq_" + shmId.shmId + "_m_" + to_string(s.first)).c_str()));
            }
        }

        unsigned int numDevices = 0;
        int creatorId = -1;
        std::string sessionName;

        DeviceCounter* deviceCounter = managementSegment.find<DeviceCounter>(unique_instance).first;
        if (deviceCounter) {
            numDevices = deviceCounter->fCount;
        }
        SessionInfo* sessionInfo = managementSegment.find<SessionInfo>(unique_instance).first;
        if (sessionInfo) {
            creatorId = sessionInfo->fCreatorId;
            sessionName = sessionInfo->fSessionName;
        }
#ifdef FAIRMQ_DEBUG_MODE
        Uint16MsgCounterHashMap* msgCounters = managementSegment.find<Uint16MsgCounterHashMap>(unique_instance).first;
#endif

        stringstream ss;
        size_t mfree = managementSegment.get_free_memory();
        size_t mtotal = managementSegment.get_size();
        size_t mused = mtotal - mfree;

        ss << "shm id: " << shmId.shmId
           << ", session: " << sessionName
           << ", creator id: " << creatorId
           << ", devices: " << numDevices
           << ", managed segments:\n";

        for (const auto& s : segments) {
            size_t free = boost::apply_visitor(SegmentFreeMemory(), s.second);
            size_t total = boost::apply_visitor(SegmentSize(), s.second);
            size_t used = total - free;

            std::string msgCount;
#ifdef FAIRMQ_DEBUG_MODE
            if (msgCounters) {
                auto it = msgCounters->find(s.first);
                if (it != msgCounters->end()) {
                    msgCount = to_string(it->second.fCount.load());
                } else {
                    msgCount = "0";
                }
            }
#else
            msgCount = "NODEBUG";
#endif

            ss << "   [" << s.first << "]"
               << ": total: " << total
               << ", msgs: " << msgCount
               << ", free: " << free
               << ", used: " << used
               << "\n";
        }

        ss << "   [m]: "
           << "total: " << mtotal
           << ", free: " << mfree
           << ", used: " << mused;

        if (shmRegions && !shmRegions->empty()) {
            ss << "\n   unmanaged regions:";
            for (const auto& r : *shmRegions) {
                ss << "\n      [" << r.first << "]: " << (r.second.fDestroyed ? "destroyed" : "alive");
                ss << ", size: " << r.second.fSize;

                // try {
                //     boost::interprocess::message_queue q(open_only, std::string("fmq_" + std::string(shmId) + "_rgq_" + to_string(r.first)).c_str());
                //     ss << ", ack queue: " << q.get_num_msg() << " messages";
                // } catch (bie&) {
                //     ss << ", ack queue: not found";
                // }
            }
        }
        LOGV(info, user1) << ss.str();
    } catch (bie&) {
        return false;
    }

    return true;
}

void Monitor::ListAll(const std::string& path)
{
    try {
        if (fs::is_empty(path)) {
            LOG(info) << "directory " << fs::path(path) << " is empty.";
            return;
        }

        for (const auto& entry : fs::directory_iterator(path)) {
            string filename = entry.path().filename().string();
            // LOG(info) << filename << ", size: " << entry.file_size() << " bytes";
            if (tools::StrStartsWith(filename, "fmq_") || tools::StrStartsWith(filename, "sem.fmq_")) {
                // LOG(info) << "The file '" << filename << "' belongs to FairMQ.";
                if (tools::StrEndsWith(filename, "_mng")) {
                    string shmId = filename.substr(4, 8);
                    LOG(info) << "\nFound shmid '" << shmId << "':\n";
                    if (!PrintShm(ShmId{shmId})) {
                        LOG(info) << "could not open file for shmid '" << shmId << "'";
                    }
                }
            } else {
                LOG(info) << "The file '" << filename << "' does not belong to FairMQ, skipping...";
            }
        }
    } catch (fs::filesystem_error& fse) {
        LOG(error) << "error: " << fse.what();
    }
}

void Monitor::CheckHeartbeats()
{
    using namespace boost::interprocess;

    uint64_t localHb = 0;

    while (!fTerminating) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        try {
            managed_shared_memory managementSegment(open_read_only, std::string("fmq_" + fShmId + "_mng").c_str());
            Heartbeat* hb = managementSegment.find<Heartbeat>(unique_instance).first;

            if (hb) {
                uint64_t globalHb = hb->fCount;
                if (localHb != globalHb) {
                    fHeartbeatTriggered = true;
                    fLastHeartbeat.store(chrono::high_resolution_clock::now());
                    localHb = globalHb;
                }
            }
        } catch (bie&) {
            // management segment not found, simply retry.
        }
    }
}

void Monitor::Interactive()
{
    char c = 0;
    pollfd cinfd[1];
    cinfd[0].fd = fileno(stdin);
    cinfd[0].events = POLLIN;

    tools::NonCanonicalInput nci;

    LOG(info) << "\n";
    PrintHelp();

    while (!fTerminating) {
        if (poll(cinfd, 1, fIntervalInMS)) {
            if (fTerminating || gSignalStatus != 0) {
                break;
            }

            c = getchar();

            switch (c) {
                case 'q':
                    LOG(info) << "\n[q] --> quitting.";
                    fTerminating = true;
                    break;
                case 'x':
                    LOG(info) << "\n[x] --> closing shared memory:";
                    if (!fViewOnly) {
                        Cleanup(ShmId{fShmId});
                    } else {
                        LOG(info) << "cannot close because in view only mode";
                    }
                    break;
                case 'h':
                    LOG(info) << "\n[h] --> help:\n";
                    PrintHelp();
                    LOG(info);
                    break;
                case '\n':
                    LOG(info) << "\n[\\n] --> invalid input.";
                    break;
                case 'b':
                    PrintDebugInfo(ShmId{fShmId});
                    break;
                default:
                    LOG(info) << "\n[" << c << "] --> invalid input.";
                    break;
            }

            if (fTerminating) {
                break;
            }
        }

        if (fTerminating) {
            break;
        }

        PrintShm(ShmId{fShmId});
    }
}

void Monitor::PrintDebugInfo(const ShmId& shmId __attribute__((unused)))
{
#ifdef FAIRMQ_DEBUG_MODE
    string managementSegmentName("fmq_" + shmId.shmId + "_mng");
    try {
        bipc::managed_shared_memory managementSegment(bipc::open_only, managementSegmentName.c_str());
        bipc::interprocess_mutex* mtx(managementSegment.find_or_construct<bipc::interprocess_mutex>(bipc::unique_instance)());
        bipc::scoped_lock<bipc::interprocess_mutex> lock(*mtx);

        Uint16MsgDebugMapHashMap* debug = managementSegment.find<Uint16MsgDebugMapHashMap>(bipc::unique_instance).first;

        size_t numMessages = 0;

        if (debug) {
            for (const auto& e : *debug) {
                numMessages += e.second.size();
            }
            LOG(info) << endl << "found " << numMessages << " messages.";

            for (const auto& s : *debug) {
                for (const auto& e : s.second) {
                    using time_point = chrono::system_clock::time_point;
                    time_point tmpt{chrono::duration_cast<time_point::duration>(chrono::nanoseconds(e.second.fCreationTime))};
                    time_t t = chrono::system_clock::to_time_t(tmpt);
                    uint64_t ms = e.second.fCreationTime % 1000000;
                    auto tm = localtime(&t);
                    LOG(info) << "segment: " << setw(3) << setfill(' ') << s.first
                        << ", offset: " << setw(12) << setfill(' ') << e.first
                        << ", size: " << setw(10) << setfill(' ') << e.second.fSize
                        << ", creator PID: " << e.second.fPid << setfill('0')
                        << ", at: " << setw(2) << tm->tm_hour << ":" << setw(2) << tm->tm_min << ":" << setw(2) << tm->tm_sec << "." << setw(6) << ms;
                }
            }
        } else {
            LOG(info) << "no debug data found";
        }
    } catch (bie&) {
        LOG(info) << "no segments found";
    }
#else
    LOG(info) << "FairMQ was not compiled in debug mode (FAIRMQ_DEBUG_MODE)";
#endif
}

void Monitor::PrintDebugInfo(const SessionId& sessionId)
{
    ShmId shmId{makeShmIdStr(sessionId.sessionId)};
    PrintDebugInfo(shmId);
}

unordered_map<uint16_t, std::vector<BufferDebugInfo>> Monitor::GetDebugInfo(const ShmId& shmId __attribute__((unused)))
{
    unordered_map<uint16_t, std::vector<BufferDebugInfo>> result;

#ifdef FAIRMQ_DEBUG_MODE
    string managementSegmentName("fmq_" + shmId.shmId + "_mng");
    try {
        bipc::managed_shared_memory managementSegment(bipc::open_only, managementSegmentName.c_str());
        bipc::interprocess_mutex* mtx(managementSegment.find_or_construct<bipc::interprocess_mutex>(bipc::unique_instance)());
        bipc::scoped_lock<bipc::interprocess_mutex> lock(*mtx);

        Uint16MsgDebugMapHashMap* debug = managementSegment.find<Uint16MsgDebugMapHashMap>(bipc::unique_instance).first;

        result.reserve(debug->size());


        if (debug) {
            for (const auto& s : *debug) {
                result[s.first].reserve(s.second.size());
                for (const auto& e : s.second) {
                    result[s.first][e.first] = BufferDebugInfo(e.first, e.second.fPid, e.second.fSize, e.second.fCreationTime);
                }
            }
        } else {
            LOG(info) << "no debug data found";
        }
    } catch (bie&) {
        LOG(info) << "no segments found";
    }
#else
    LOG(info) << "FairMQ was not compiled in debug mode (FAIRMQ_DEBUG_MODE)";
#endif

    return result;
}
unordered_map<uint16_t, std::vector<BufferDebugInfo>> Monitor::GetDebugInfo(const SessionId& sessionId)
{
    ShmId shmId{makeShmIdStr(sessionId.sessionId)};
    return GetDebugInfo(shmId);
}

unsigned long Monitor::GetFreeMemory(const ShmId& shmId, uint16_t segmentId)
{
    using namespace boost::interprocess;
    try {
        bipc::managed_shared_memory managementSegment(bipc::open_only, std::string("fmq_" + shmId.shmId + "_mng").c_str());
        boost::interprocess::interprocess_mutex* mtx(managementSegment.find_or_construct<bipc::interprocess_mutex>(bipc::unique_instance)());
        boost::interprocess::scoped_lock<bipc::interprocess_mutex> lock(*mtx);

        Uint16SegmentInfoHashMap* shmSegments = managementSegment.find<Uint16SegmentInfoHashMap>(unique_instance).first;

        if (!shmSegments) {
            LOG(error) << "Found management segment, but could not locate segment info";
            throw MonitorError("Found management segment, but could not locate segment info");
        }

        auto it = shmSegments->find(segmentId);
        if (it != shmSegments->end()) {
            if (it->second.fAllocationAlgorithm == AllocationAlgorithm::rbtree_best_fit) {
                RBTreeBestFitSegment segment(open_read_only, std::string("fmq_" + shmId.shmId + "_m_" + std::to_string(segmentId)).c_str());
                return segment.get_free_memory();
            } else {
                SimpleSeqFitSegment segment(open_read_only, std::string("fmq_" + shmId.shmId + "_m_" + std::to_string(segmentId)).c_str());
                return segment.get_free_memory();
            }
        } else {
            LOG(error) << "Could not find segment id '" << segmentId << "'";
            throw MonitorError(tools::ToString("Could not find segment id '", segmentId, "'"));
        }
    } catch (bie&) {
        LOG(error) << "Could not find management segment for shmid '" << shmId.shmId << "'";
        throw MonitorError(tools::ToString("Could not find management segment for shmid '", shmId.shmId, "'"));
    }
}
unsigned long Monitor::GetFreeMemory(const SessionId& sessionId, uint16_t segmentId)
{
    ShmId shmId{makeShmIdStr(sessionId.sessionId)};
    return GetFreeMemory(shmId, segmentId);
}

bool Monitor::SegmentIsPresent(const ShmId& shmId, uint16_t segmentId)
{
    using namespace boost::interprocess;
    try {
        bipc::managed_shared_memory managementSegment(bipc::open_read_only, std::string("fmq_" + shmId.shmId + "_mng").c_str());
        Uint16SegmentInfoHashMap* shmSegments = managementSegment.find<Uint16SegmentInfoHashMap>(unique_instance).first;

        if (!shmSegments) {
            LOG(error) << "Found management segment, but could not locate segment info";
            return false;
        }

        auto it = shmSegments->find(segmentId);
        if (it != shmSegments->end()) {
            try {
                if (it->second.fAllocationAlgorithm == AllocationAlgorithm::rbtree_best_fit) {
                    RBTreeBestFitSegment segment(open_read_only, std::string("fmq_" + shmId.shmId + "_m_" + std::to_string(segmentId)).c_str());
                } else {
                    SimpleSeqFitSegment segment(open_read_only, std::string("fmq_" + shmId.shmId + "_m_" + std::to_string(segmentId)).c_str());
                }
            } catch (bie&) {
                LOG(error) << "Could not find segment with id '" << segmentId << "' for shmId '" << shmId.shmId << "'";
                return false;
            }
        } else {
            LOG(error) << "Could not find segment info for segment id '" << segmentId << "' for shmId '" << shmId.shmId << "'";
            return false;
        }
    } catch (bie&) {
        LOG(error) << "Could not find management segment for shmid '" << shmId.shmId << "'";
        return false;
    }

    return true;
}
bool Monitor::SegmentIsPresent(const SessionId& sessionId, uint16_t segmentId)
{
    ShmId shmId{makeShmIdStr(sessionId.sessionId)};
    return SegmentIsPresent(shmId, segmentId);
}

bool Monitor::RegionIsPresent(const ShmId& shmId, uint16_t regionId)
{
    using namespace boost::interprocess;
    try {
        bipc::managed_shared_memory managementSegment(bipc::open_read_only, std::string("fmq_" + shmId.shmId + "_mng").c_str());
        Uint16RegionInfoHashMap* shmRegions = managementSegment.find<Uint16RegionInfoHashMap>(bipc::unique_instance).first;

        if (!shmRegions) {
            LOG(error) << "Found management segment, but could not locate region info";
            return false;
        }

        std::string regionFileName("fmq_" + shmId.shmId + "_rg_" + to_string(regionId));

        auto it = shmRegions->find(regionId);
        if (it != shmRegions->end()) {
            try {
                if (it->second.fPath.empty()) {
                    shared_memory_object object(open_only, regionFileName.c_str(), read_only);
                }
            } catch (bie&) {
                LOG(error) << "Could not find region with id '" << regionId << "' for shmId '" << shmId.shmId << "'";
                return false;
            }
        } else {
            LOG(error) << "Could not find region info for region id '" << regionId << "' for shmId '" << shmId.shmId << "'";
            return false;
        }
    } catch (bie&) {
        LOG(error) << "Could not find management segment for shmid '" << shmId.shmId << "'";
        return false;
    }

    return true;
}
bool Monitor::RegionIsPresent(const SessionId& sessionId, uint16_t regionId)
{
    ShmId shmId{makeShmIdStr(sessionId.sessionId)};
    return RegionIsPresent(shmId, regionId);
}

void Monitor::PrintHelp()
{
    LOG(info) << "controls: [x] close memory, "
              << "[b] print a list of allocated messages (only available when compiled with FAIMQ_DEBUG_MODE=ON), "
              << "[h] help, "
              << "[q] quit.";
}

bool Monitor::RemoveObject(const string& name)      { return bipc::shared_memory_object::remove(name.c_str()); }
bool Monitor::RemoveFileMapping(const string& name) { return bipc::file_mapping::remove(name.c_str()); }
bool Monitor::RemoveQueue(const string& name)       { return bipc::message_queue::remove(name.c_str()); }
bool Monitor::RemoveMutex(const string& name)       { return bipc::named_mutex::remove(name.c_str()); }
bool Monitor::RemoveCondition(const string& name)   { return bipc::named_condition::remove(name.c_str()); }

template<typename T>
std::pair<std::string, bool> Remove(const std::string& name, bool verbose)
{
    if (T::remove(name.c_str())) {
        if (verbose) {
            LOG(info) << "Successfully removed '" << name << "'.";
        }
        return {name, true};
    } else {
        if (verbose) {
            LOG(debug) << "Did not remove '" << name << "'. Already removed?";
        }
        return {name, false};
    }
}

std::vector<std::pair<std::string, bool>> Monitor::Cleanup(const ShmId& shmIdT, bool verbose /* = true */)
{
    std::string shmId = shmIdT.shmId;
    std::vector<std::pair<std::string, bool>> result;

    if (verbose) {
        LOG(info) << "Cleaning up for shared memory id '" << shmId << "'...";
    }

    string managementSegmentName("fmq_" + shmId + "_mng");
    try {
        bipc::managed_shared_memory managementSegment(bipc::open_read_only, managementSegmentName.c_str());

        Uint16RegionInfoHashMap* shmRegions = managementSegment.find<Uint16RegionInfoHashMap>(bipc::unique_instance).first;
        if (shmRegions) {
            if (verbose) {
                LOG(info) << "Found " << shmRegions->size() << " unmanaged regions...";
            }
            for (const auto& region : *shmRegions) {
                uint16_t id = region.first;
                RegionInfo info = region.second;
                string path = info.fPath.c_str();
                int flags = info.fCreationFlags;
                if (verbose) {
                    LOG(info) << "Found RegionInfo with path: '" << path << "', flags: " << flags << ", fDestroyed: " << info.fDestroyed << ".";
                }
                if (!path.empty()) {
                    result.emplace_back(Remove<bipc::file_mapping>(path + "fmq_" + shmId + "_rg_" + to_string(id), verbose));
                } else {
                    result.emplace_back(Remove<bipc::shared_memory_object>("fmq_" + shmId + "_rg_" + to_string(id), verbose));
                }
                result.emplace_back(Remove<bipc::message_queue>("fmq_" + shmId + "_rgq_" + to_string(id), verbose));
            }
        }

        Uint16SegmentInfoHashMap* shmSegments = managementSegment.find<Uint16SegmentInfoHashMap>(bipc::unique_instance).first;
        if (shmSegments) {
            if (verbose) {
                LOG(info) << "Found " << shmSegments->size() << " managed segments...";
            }
            for (const auto& segment : *shmSegments) {
                result.emplace_back(Remove<bipc::shared_memory_object>("fmq_" + shmId + "_m_" + to_string(segment.first), verbose));
            }
        } else {
            if (verbose) {
                LOG(info) << "Cannot find information about managed segments";
            }
        }

        result.emplace_back(Remove<bipc::shared_memory_object>(managementSegmentName, verbose));
    } catch (bie&) {
        if (verbose) {
            LOG(info) << "Did not find '" << managementSegmentName << "' management segment. No regions to cleanup.";
        }
    }

    return result;
}

std::vector<std::pair<std::string, bool>> Monitor::Cleanup(const SessionId& sessionId, bool verbose /* = true */)
{
    ShmId shmId{makeShmIdStr(sessionId.sessionId)};
    if (verbose) {
        LOG(info) << "Cleanup called with session id '" << sessionId.sessionId << "', translating to shared memory id '" << shmId.shmId << "'";
    }
    return Cleanup(shmId, verbose);
}

std::vector<std::pair<std::string, bool>> Monitor::CleanupFull(const ShmId& shmId, bool verbose /* = true */)
{
    auto result = Cleanup(shmId, verbose);
    result.emplace_back(Remove<bipc::named_mutex>("fmq_" + shmId.shmId + "_ms", verbose));
    return result;
}

std::vector<std::pair<std::string, bool>> Monitor::CleanupFull(const SessionId& sessionId, bool verbose /* = true */)
{
    ShmId shmId{makeShmIdStr(sessionId.sessionId)};
    if (verbose) {
        LOG(info) << "Cleanup called with session id '" << sessionId.sessionId << "', translating to shared memory id '" << shmId.shmId << "'";
    }
    return CleanupFull(shmId, verbose);
}

void Monitor::ResetContent(const ShmId& shmIdT, bool verbose /* = true */)
{
    std::string shmId = shmIdT.shmId;
    if (verbose) {
        cout << "Resetting segments content for shared memory id '" << shmId << "'..." << endl;
    }

    string managementSegmentName("fmq_" + shmId + "_mng");
    try {
        using namespace boost::interprocess;
        managed_shared_memory managementSegment(open_only, managementSegmentName.c_str());

        Uint16SegmentInfoHashMap* segmentInfos = managementSegment.find<Uint16SegmentInfoHashMap>(unique_instance).first;
        if (segmentInfos) {
            cout << "Found info for " << segmentInfos->size() << " managed segments" << endl;
            for (const auto& s : *segmentInfos) {
                if (verbose) {
                    cout << "Resetting content of segment '" << "fmq_" << shmId << "_m_" << s.first << "'..." << endl;
                }
                try {
                    if (s.second.fAllocationAlgorithm == AllocationAlgorithm::rbtree_best_fit) {
                        RBTreeBestFitSegment segment(open_only, std::string("fmq_" + shmId + "_m_" + to_string(s.first)).c_str());
                        void* ptr = segment.get_segment_manager();
                        size_t size = segment.get_segment_manager()->get_size();
                        new(ptr) segment_manager<char, rbtree_best_fit<mutex_family, offset_ptr<void>>, null_index>(size);
                    } else {
                        SimpleSeqFitSegment segment(open_only, std::string("fmq_" + shmId + "_m_" + to_string(s.first)).c_str());
                        void* ptr = segment.get_segment_manager();
                        size_t size = segment.get_segment_manager()->get_size();
                        new(ptr) segment_manager<char, simple_seq_fit<mutex_family, offset_ptr<void>>, null_index>(size);
                    }
                    if (verbose) {
                        cout << "Done." << endl;
                    }
                } catch (bie& e) {
                    if (verbose) {
                        cout << "Error resetting content of segment '" << std::string("fmq_" + shmId + "_m_" + to_string(s.first)) << "': " << e.what() << endl;
                    }
                }
            }
        } else {
            cout << "Found management segment, but cannot locate segment info, something went wrong..." << endl;
        }

        Uint16RegionInfoHashMap* shmRegions = managementSegment.find<Uint16RegionInfoHashMap>(bipc::unique_instance).first;
        if (shmRegions) {
            for (const auto& region : *shmRegions) {
                uint16_t id = region.first;
                Remove<bipc::message_queue>("fmq_" + shmId + "_rgq_" + to_string(id), verbose);
            }
        }
    } catch (bie& e) {
        if (verbose) {
            cout << "Could not find '" << managementSegmentName << "' segment. Nothing to cleanup." << endl;
            cout << e.what() << endl;
        }
    }

    if (verbose) {
        cout << "Done resetting segment content for shared memory id '" << shmId << "'." << endl;
    }
}

void Monitor::ResetContent(const SessionId& sessionId, bool verbose /* = true */)
{
    ShmId shmId{makeShmIdStr(sessionId.sessionId)};
    if (verbose) {
        cout << "ResetContent called with session id '" << sessionId.sessionId << "', translating to shared memory id '" << shmId.shmId << "'" << endl;
    }
    ResetContent(shmId, verbose);
}

void Monitor::ResetContent(const ShmId& shmIdT, const std::vector<SegmentConfig>& segmentCfgs, const std::vector<RegionConfig>& regionCfgs, bool verbose /* = true */)
{
    using namespace boost::interprocess;

    std::string shmId = shmIdT.shmId;
    std::string managementSegmentName("fmq_" + shmId + "_mng");
    // delete management segment
    cout << "deleting management segment" << endl;
    Remove<bipc::shared_memory_object>(managementSegmentName, verbose);
    // recreate management segment
    cout << "recreating management segment..." << endl;
    managed_shared_memory mngSegment(create_only, managementSegmentName.c_str(), kManagementSegmentSize);
    cout << "done." << endl;
    // fill management segment with segment & region infos
    cout << "filling management segment with managed segment configs..." << endl;
    for (const auto& s : segmentCfgs) {
        if (s.allocationAlgorithm == "rbtree_best_fit") {
            Segment::Register(shmId, s.id, AllocationAlgorithm::rbtree_best_fit);
        } else if (s.allocationAlgorithm == "simple_seq_fit") {
            Segment::Register(shmId, s.id, AllocationAlgorithm::simple_seq_fit);
        } else {
            LOG(error) << "Unknown allocation algorithm provided: " << s.allocationAlgorithm;
            throw MonitorError("Unknown allocation algorithm provided: " + s.allocationAlgorithm);
        }
    }
    cout << "done." << endl;
    cout << "filling management segment with unmanaged region configs..." << endl;
    for (const auto& r : regionCfgs) {
        fair::mq::shmem::UnmanagedRegion::Register(shmId, r);
    }
    cout << "done." << endl;
    // reset managed segments
    ResetContent(shmIdT, verbose);
}

void Monitor::ResetContent(const SessionId& sessionId, const std::vector<SegmentConfig>& segmentCfgs, const std::vector<RegionConfig>& regionCfgs, bool verbose /* = true */)
{
    ShmId shmId{makeShmIdStr(sessionId.sessionId)};
    if (verbose) {
        cout << "ResetContent called with session id '" << sessionId.sessionId << "', translating to shared memory id '" << shmId.shmId << "'" << endl;
    }
    ResetContent(shmId, segmentCfgs, regionCfgs, verbose);
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

} // namespace fair::mq::shmem

/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Manager.h
 *
 * @since 2016-04-08
 * @author A. Rybalchenko
 */

#ifndef FAIR_MQ_SHMEM_MANAGER_H_
#define FAIR_MQ_SHMEM_MANAGER_H_

#include "Common.h"
#include "Monitor.h"
#include "Region.h"

#include <fairmq/ProgOptions.h>
#include <fairmq/tools/Strings.h>
#include <fairmq/Transports.h>
#include <FairMQLogger.h>
#include <FairMQMessage.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/variant.hpp>

#include <condition_variable>
#include <cstdlib> // getenv
#include <memory> // make_unique
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility> // pair
#include <vector>

#include <unistd.h> // getuid
#include <sys/types.h> // getuid

#include <sys/mman.h> // mlock

namespace fair::mq::shmem
{

class Manager
{
  public:
    Manager(const std::string& sessionName, std::string deviceId, size_t size, const ProgOptions* config)
        : fShmId64(makeShmIdUint64(sessionName))
        , fShmId(makeShmIdStr(sessionName))
        , fSegmentId(config ? config->GetProperty<uint16_t>("shm-segment-id", 0) : 0)
        , fDeviceId(std::move(deviceId))
        , fManagementSegment(boost::interprocess::open_or_create, std::string("fmq_" + fShmId + "_mng").c_str(), 6553600)
        , fShmVoidAlloc(fManagementSegment.get_segment_manager())
        , fShmMtx(boost::interprocess::open_or_create, std::string("fmq_" + fShmId + "_mtx").c_str())
        , fRegionEventsShmCV(boost::interprocess::open_or_create, std::string("fmq_" + fShmId + "_cv").c_str())
        , fRegionEventsSubscriptionActive(false)
        , fNumObservedEvents(0)
        , fDeviceCounter(nullptr)
        , fEventCounter(nullptr)
        , fShmSegments(nullptr)
        , fShmRegions(nullptr)
        , fInterrupted(false)
#ifdef FAIRMQ_DEBUG_MODE
        , fMsgDebug(nullptr)
        , fShmMsgCounters(nullptr)
        , fMsgCounterNew(0)
        , fMsgCounterDelete(0)
#endif
        , fBeatTheHeart(true)
        , fThrowOnBadAlloc(config ? config->GetProperty<bool>("shm-throw-bad-alloc", true) : true)
        , fNoCleanup(config ? config->GetProperty<bool>("shm-no-cleanup", false) : false)
    {
        using namespace boost::interprocess;

        LOG(debug) << "Generated shmid '" << fShmId << "' out of session id '" << sessionName << "'.";

        bool mlockSegment = false;
        bool zeroSegment = false;
        bool autolaunchMonitor = false;
        std::string allocationAlgorithm("rbtree_best_fit");
        if (config) {
            mlockSegment = config->GetProperty<bool>("shm-mlock-segment", mlockSegment);
            zeroSegment = config->GetProperty<bool>("shm-zero-segment", zeroSegment);
            autolaunchMonitor = config->GetProperty<bool>("shm-monitor", autolaunchMonitor);
            allocationAlgorithm = config->GetProperty<std::string>("shm-allocation", allocationAlgorithm);
        } else {
            LOG(debug) << "ProgOptions not available! Using defaults.";
        }

        if (autolaunchMonitor) {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);
            StartMonitor(fShmId);
        }

        fHeartbeatThread = std::thread(&Manager::Heartbeats, this);

        try {
            std::stringstream ss;
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);

            fShmSegments = fManagementSegment.find_or_construct<Uint16SegmentInfoHashMap>(unique_instance)(fShmVoidAlloc);

            SessionInfo* sessionInfo = fManagementSegment.find<SessionInfo>(unique_instance).first;
            if (sessionInfo) {
                LOG(debug) << "session info found, name: " << sessionInfo->fSessionName << ", creator id: " << sessionInfo->fCreatorId;
            } else {
                LOG(debug) << "no session info found, creating and initializing";
                sessionInfo = fManagementSegment.construct<SessionInfo>(unique_instance)(sessionName.c_str(), geteuid(), fShmVoidAlloc);
                LOG(debug) << "initialized session info, name: " << sessionInfo->fSessionName << ", creator id: " << sessionInfo->fCreatorId;
            }

            fEventCounter = fManagementSegment.find<EventCounter>(unique_instance).first;
            if (fEventCounter) {
                LOG(debug) << "event counter found: " << fEventCounter->fCount;
            } else {
                LOG(debug) << "no event counter found, creating one and initializing with 0";
                fEventCounter = fManagementSegment.construct<EventCounter>(unique_instance)(0);
                LOG(debug) << "initialized event counter with: " << fEventCounter->fCount;
            }

            fShmRegions = fManagementSegment.find_or_construct<Uint16RegionInfoHashMap>(unique_instance)(fShmVoidAlloc);

            fDeviceCounter = fManagementSegment.find<DeviceCounter>(unique_instance).first;
            if (fDeviceCounter) {
                LOG(debug) << "device counter found, with value of " << fDeviceCounter->fCount << ". incrementing.";
                (fDeviceCounter->fCount)++;
                LOG(debug) << "incremented device counter, now: " << fDeviceCounter->fCount;
            } else {
                LOG(debug) << "no device counter found, creating one and initializing with 1";
                fDeviceCounter = fManagementSegment.construct<DeviceCounter>(unique_instance)(1);
                LOG(debug) << "initialized device counter with: " << fDeviceCounter->fCount;
            }

            std::string op("create/open");

            try {
                auto it = fShmSegments->find(fSegmentId);
                if (it == fShmSegments->end()) {
                    op = "create";
                    // no segment with given id exists, creating
                    if (allocationAlgorithm == "rbtree_best_fit") {
                        fSegments.emplace(fSegmentId, RBTreeBestFitSegment(create_only, std::string("fmq_" + fShmId + "_m_" + std::to_string(fSegmentId)).c_str(), size));
                        fShmSegments->emplace(fSegmentId, AllocationAlgorithm::rbtree_best_fit);
                    } else if (allocationAlgorithm == "simple_seq_fit") {
                        fSegments.emplace(fSegmentId, SimpleSeqFitSegment(create_only, std::string("fmq_" + fShmId + "_m_" + std::to_string(fSegmentId)).c_str(), size));
                        fShmSegments->emplace(fSegmentId, AllocationAlgorithm::simple_seq_fit);
                    }
                    ss << "Created ";
                    (fEventCounter->fCount)++;
                } else {
                    op = "open";
                    // found segment with the given id, opening
                    if (it->second.fAllocationAlgorithm == AllocationAlgorithm::rbtree_best_fit) {
                        fSegments.emplace(fSegmentId, RBTreeBestFitSegment(open_only, std::string("fmq_" + fShmId + "_m_" + std::to_string(fSegmentId)).c_str()));
                        if (allocationAlgorithm != "rbtree_best_fit") {
                            LOG(warn) << "Allocation algorithm of the opened segment is rbtree_best_fit, but requested is " << allocationAlgorithm << ". Ignoring requested setting.";
                            allocationAlgorithm = "rbtree_best_fit";
                        }
                    } else {
                        fSegments.emplace(fSegmentId, SimpleSeqFitSegment(open_only, std::string("fmq_" + fShmId + "_m_" + std::to_string(fSegmentId)).c_str()));
                        if (allocationAlgorithm != "simple_seq_fit") {
                            LOG(warn) << "Allocation algorithm of the opened segment is simple_seq_fit, but requested is " << allocationAlgorithm << ". Ignoring requested setting.";
                            allocationAlgorithm = "simple_seq_fit";
                        }
                    }
                    ss << "Opened ";
                }
                ss << "shared memory segment '" << "fmq_" << fShmId << "_m_" << fSegmentId << "'."
                << " Size: " << boost::apply_visitor(SegmentSize(), fSegments.at(fSegmentId)) << " bytes."
                << " Available: " << boost::apply_visitor(SegmentFreeMemory(), fSegments.at(fSegmentId)) << " bytes."
                << " Allocation algorithm: " << allocationAlgorithm;
                LOG(debug) << ss.str();
            } catch(interprocess_exception& bie) {
                LOG(error) << "Failed to " << op << " shared memory segment (" << "fmq_" << fShmId << "_m_" << fSegmentId << "): " << bie.what();
                throw TransportError(tools::ToString("Failed to ", op, " shared memory segment (", "fmq_", fShmId, "_m_", fSegmentId, "): ", bie.what()));
            }

            if (mlockSegment) {
                LOG(debug) << "Locking the managed segment memory pages...";
                if (mlock(boost::apply_visitor(SegmentAddress(), fSegments.at(fSegmentId)), boost::apply_visitor(SegmentSize(), fSegments.at(fSegmentId))) == -1) {
                    LOG(error) << "Could not lock the managed segment memory. Code: " << errno << ", reason: " << strerror(errno);
                    throw TransportError(tools::ToString("Could not lock the managed segment memory: ", strerror(errno)));
                }
                LOG(debug) << "Successfully locked the managed segment memory pages.";
            }
            if (zeroSegment) {
                LOG(debug) << "Zeroing the managed segment free memory...";
                boost::apply_visitor(SegmentMemoryZeroer(), fSegments.at(fSegmentId));
                LOG(debug) << "Successfully zeroed the managed segment free memory.";
            }

#ifdef FAIRMQ_DEBUG_MODE
            fMsgDebug = fManagementSegment.find_or_construct<Uint16MsgDebugMapHashMap>(unique_instance)(fShmVoidAlloc);
            fShmMsgCounters = fManagementSegment.find_or_construct<Uint16MsgCounterHashMap>(unique_instance)(fShmVoidAlloc);
#endif
        } catch (...) {
            StopHeartbeats();
            CleanupIfLast();
            throw;
        }
    }

    Manager() = delete;

    Manager(const Manager&) = delete;
    Manager operator=(const Manager&) = delete;

  private:
    static bool SpawnShmMonitor(const std::string& id);

  public:
    static void StartMonitor(const std::string& id)
    {
        using namespace boost::interprocess;
        try {
            named_mutex monitorStatus(open_only, std::string("fmq_" + id + "_ms").c_str());
            LOG(debug) << "Found fairmq-shmmonitor for shared memory id " << id;
        } catch (interprocess_exception&) {
            LOG(debug) << "no fairmq-shmmonitor found for shared memory id " << id << ", starting...";

            if (SpawnShmMonitor(id)) {
                int numTries = 0;
                do {
                    try {
                        named_mutex monitorStatus(open_only, std::string("fmq_" + id + "_ms").c_str());
                        LOG(debug) << "Started fairmq-shmmonitor for shared memory id " << id;
                        break;
                    } catch (interprocess_exception&) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        if (++numTries > 1000) {
                            LOG(error) << "Did not get response from fairmq-shmmonitor after " << 10 * 1000 << " milliseconds. Exiting.";
                            throw std::runtime_error(tools::ToString("Did not get response from fairmq-shmmonitor after ", 10 * 1000, " milliseconds. Exiting."));
                        }
                    }
                } while (true);
            }
        }
    }

    void Interrupt() { fInterrupted.store(true); }
    void Resume() { fInterrupted.store(false); }
    void Reset()
    {
#ifdef FAIRMQ_DEBUG_MODE
        auto diff = fMsgCounterNew.load() - fMsgCounterDelete.load();
        if (diff != 0) {
            LOG(error) << "Message counter during Reset expected to be 0, found: " << diff;
            throw MessageError(tools::ToString("Message counter during Reset expected to be 0, found: ", diff));
        }
#endif
    }
    bool Interrupted() { return fInterrupted.load(); }

    std::pair<boost::interprocess::mapped_region*, uint16_t> CreateRegion(const size_t size,
                                                                          const int64_t userFlags,
                                                                          RegionCallback callback,
                                                                          RegionBulkCallback bulkCallback,
                                                                          const std::string& path,
                                                                          int flags,
                                                                          fair::mq::RegionConfig cfg)
    {
        using namespace boost::interprocess;
        try {
            std::pair<mapped_region*, uint16_t> result;

            {
                uint16_t id = 0;
                boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);

                RegionCounter* rc = fManagementSegment.find<RegionCounter>(unique_instance).first;

                if (rc) {
                    LOG(debug) << "region counter found, with value of " << rc->fCount << ". incrementing.";
                    (rc->fCount)++;
                    LOG(debug) << "incremented region counter, now: " << rc->fCount;
                } else {
                    LOG(debug) << "no region counter found, creating one and initializing with 1";
                    rc = fManagementSegment.construct<RegionCounter>(unique_instance)(1);
                    LOG(debug) << "initialized region counter with: " << rc->fCount;
                }

                id = rc->fCount;

                auto it = fRegions.find(id);
                if (it != fRegions.end()) {
                    LOG(error) << "Trying to create a region that already exists";
                    return {nullptr, id};
                }

                auto r = fRegions.emplace(id, std::make_unique<Region>(fShmId, id, size, false, callback, bulkCallback, path, flags));
                // LOG(debug) << "Created region with id '" << id << "', path: '" << path << "', flags: '" << flags << "'";

                if (cfg.lock) {
                    LOG(debug) << "Locking region " << id << "...";
                    if (mlock(r.first->second->fRegion.get_address(), r.first->second->fRegion.get_size()) == -1) {
                        LOG(error) << "Could not lock region " << id << ". Code: " << errno << ", reason: " << strerror(errno);
                        throw TransportError(tools::ToString("Could not lock region ", id, ": ", strerror(errno)));
                    }
                    LOG(debug) << "Successfully locked region " << id << ".";
                }
                if (cfg.zero) {
                    LOG(debug) << "Zeroing free memory of region " << id << "...";
                    memset(r.first->second->fRegion.get_address(), 0x00, r.first->second->fRegion.get_size());
                    LOG(debug) << "Successfully zeroed free memory of region " << id << ".";
                }

                fShmRegions->emplace(id, RegionInfo(path.c_str(), flags, userFlags, fShmVoidAlloc));

                r.first->second->StartReceivingAcks();
                result.first = &(r.first->second->fRegion);
                result.second = id;

                (fEventCounter->fCount)++;
            }
            fRegionsGen += 1; // signal TL cache invalidation
            fRegionEventsShmCV.notify_all();

            return result;
        } catch (interprocess_exception& e) {
            LOG(error) << "cannot create region. Already created/not cleaned up?";
            LOG(error) << e.what();
            throw;
        }
    }

    Region* GetRegion(const uint16_t id)
    {
        // NOTE: gcc optimizations. Prevent loading tls addresses many times in the fast path
        const auto &lTlCache = fTlRegionCache;
        const auto &lTlCacheVec = lTlCache.fRegionsTLCache;
        const auto lTlCacheGen = lTlCache.fRegionsTLCacheGen;

        // fast path
        for (const auto &lRegion : lTlCacheVec) {
            if ((std::get<1>(lRegion) == id) && (lTlCacheGen == fRegionsGen) && (std::get<2>(lRegion) == fShmId64)) {
                return std::get<0>(lRegion);
            }
        }

        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);
        // slow path: check invalidation
        if (lTlCacheGen != fRegionsGen) {
            fTlRegionCache.fRegionsTLCache.clear();
        }

        auto *lRegion = GetRegionUnsafe(id);
        fTlRegionCache.fRegionsTLCache.emplace_back(std::make_tuple(lRegion, id, fShmId64));
        fTlRegionCache.fRegionsTLCacheGen = fRegionsGen;
        return lRegion;
    }

    Region* GetRegionUnsafe(const uint16_t id)
    {
        // remote region could actually be a local one if a message originates from this device (has been sent out and returned)
        auto it = fRegions.find(id);
        if (it != fRegions.end()) {
            return it->second.get();
        } else {
            try {
                // get region info
                RegionInfo regionInfo = fShmRegions->at(id);
                std::string path = regionInfo.fPath.c_str();
                int flags = regionInfo.fFlags;
                // LOG(debug) << "Located remote region with id '" << id << "', path: '" << path << "', flags: '" << flags << "'";

                auto r = fRegions.emplace(id, std::make_unique<Region>(fShmId, id, 0, true, nullptr, nullptr, path, flags));
                return r.first->second.get();
            } catch (std::out_of_range& oor) {
                LOG(error) << "Could not get remote region with id '" << id << "'. Does the region creator run with the same session id?";
                LOG(error) << oor.what();
                return nullptr;
            } catch (boost::interprocess::interprocess_exception& e) {
                LOG(error) << "Could not get remote region for id '" << id << "': " << e.what();
                return nullptr;
            }
        }
    }

    void RemoveRegion(const uint16_t id)
    {
        try {
            fRegions.at(id)->StopAcks();
            {
                boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);
                fShmRegions->at(id).fDestroyed = true;
                fRegions.erase(id);
                (fEventCounter->fCount)++;
            }
            fRegionEventsShmCV.notify_all();
        } catch(std::out_of_range& oor) {
            LOG(debug) << "RemoveRegion() could not locate region with id '" << id << "'";
        }
        fRegionsGen += 1; // signal TL cache invalidation
    }

    std::vector<fair::mq::RegionInfo> GetRegionInfo()
    {
        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);
        return GetRegionInfoUnsafe();
    }

    std::vector<fair::mq::RegionInfo> GetRegionInfoUnsafe()
    {
        std::vector<fair::mq::RegionInfo> result;

        for (const auto& e : *fShmRegions) {
            fair::mq::RegionInfo info;
            info.managed = false;
            info.id = e.first;
            info.flags = e.second.fUserFlags;
            info.event = e.second.fDestroyed ? RegionEvent::destroyed : RegionEvent::created;
            if (!e.second.fDestroyed) {
                auto region = GetRegionUnsafe(info.id);
                if (region) {
                    info.ptr = region->fRegion.get_address();
                    info.size = region->fRegion.get_size();
                } else {
                    throw std::runtime_error(tools::ToString("GetRegionInfoUnsafe() could not get region with id '", info.id, "'"));
                }
            } else {
                info.ptr = nullptr;
                info.size = 0;
            }
            result.push_back(info);
        }

        for (const auto& e : *fShmSegments) {
            // make sure any segments in the session are found
            GetSegment(e.first);
            try {
                fair::mq::RegionInfo info;
                info.managed = true;
                info.id = e.first;
                info.event = RegionEvent::created;
                info.ptr = boost::apply_visitor(SegmentAddress(), fSegments.at(e.first));
                info.size = boost::apply_visitor(SegmentSize(), fSegments.at(e.first));
                result.push_back(info);
            } catch (const std::out_of_range& oor) {
                LOG(error) << "could not find segment with id " << e.first;
                LOG(error) << oor.what();
            }
        }

        return result;
    }

    void SubscribeToRegionEvents(RegionEventCallback callback)
    {
        if (fRegionEventThread.joinable()) {
            LOG(debug) << "Already subscribed. Overwriting previous subscription.";
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);
            fRegionEventsSubscriptionActive = false;
            lock.unlock();
            fRegionEventsShmCV.notify_all();
            fRegionEventThread.join();
        }
        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);
        fRegionEventCallback = callback;
        fRegionEventsSubscriptionActive = true;
        fRegionEventThread = std::thread(&Manager::RegionEventsSubscription, this);
    }

    bool SubscribedToRegionEvents() { return fRegionEventThread.joinable(); }

    void UnsubscribeFromRegionEvents()
    {
        if (fRegionEventThread.joinable()) {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);
            fRegionEventsSubscriptionActive = false;
            lock.unlock();
            fRegionEventsShmCV.notify_all();
            fRegionEventThread.join();
            lock.lock();
            fRegionEventCallback = nullptr;
        }
    }

    void RegionEventsSubscription()
    {
        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);
        while (fRegionEventsSubscriptionActive) {
            auto infos = GetRegionInfoUnsafe();
            for (const auto& i : infos) {
                auto el = fObservedRegionEvents.find({i.id, i.managed});
                if (el == fObservedRegionEvents.end()) { // if event id has not been observed
                    fObservedRegionEvents.emplace(std::make_pair(i.id, i.managed), i.event);
                    // if a region has been created and destroyed rapidly, we could see 'destroyed' without ever seeing 'created'
                    // TODO: do we care to show 'created' events if we know region is already destroyed?
                    if (i.event == RegionEvent::created) {
                        fRegionEventCallback(i);
                        ++fNumObservedEvents;
                    } else {
                        fNumObservedEvents += 2;
                    }
                } else { // if event id has been observed (expected - there are two events per id - created & destroyed)
                    // fire a callback if we have observed 'created' event and incoming is 'destroyed'
                    if (el->second == RegionEvent::created && i.event == RegionEvent::destroyed) {
                        fRegionEventCallback(i);
                        el->second = i.event;
                        ++fNumObservedEvents;
                    } else {
                        // LOG(debug) << "ignoring event " << i.id << ": incoming: " << i.event << ", stored: " << el->second;
                    }
                }
            }
            fRegionEventsShmCV.wait(lock, [&] { return !fRegionEventsSubscriptionActive || fNumObservedEvents != fEventCounter->fCount; });
        }
    }

    void IncrementMsgCounter()
    {
#ifdef FAIRMQ_DEBUG_MODE
        fMsgCounterNew.fetch_add(1, std::memory_order_relaxed);
#endif
    }
    void DecrementMsgCounter()
    {
#ifdef FAIRMQ_DEBUG_MODE
        fMsgCounterDelete.fetch_add(1, std::memory_order_relaxed);
#endif
    }

#ifdef FAIRMQ_DEBUG_MODE
    void IncrementShmMsgCounter(uint16_t segmentId) { ++((*fShmMsgCounters)[segmentId].fCount); }
    void DecrementShmMsgCounter(uint16_t segmentId) { --((*fShmMsgCounters)[segmentId].fCount); }
#endif

    void Heartbeats()
    {
        using namespace boost::interprocess;

        Heartbeat* hb = fManagementSegment.find_or_construct<Heartbeat>(unique_instance)(0);
        std::unique_lock<std::mutex> lock(fHeartbeatsMtx);
        while (fBeatTheHeart) {
            (hb->fCount)++;
            fHeartbeatsCV.wait_for(lock, std::chrono::milliseconds(100), [&]() { return !fBeatTheHeart; });
        }
    }

    void StopHeartbeats()
    {
        {
            std::unique_lock<std::mutex> lock(fHeartbeatsMtx);
            fBeatTheHeart = false;
        }
        fHeartbeatsCV.notify_one();
        if (fHeartbeatThread.joinable()) {
            fHeartbeatThread.join();
        }
    }

    bool ThrowingOnBadAlloc() const { return fThrowOnBadAlloc; }

    void GetSegment(uint16_t id)
    {
        auto it = fSegments.find(id);
        if (it == fSegments.end()) {
            try {
                // get segment info
                SegmentInfo segmentInfo = fShmSegments->at(id);
                LOG(debug) << "Located segment with id '" << id << "'";

                using namespace boost::interprocess;

                if (segmentInfo.fAllocationAlgorithm == AllocationAlgorithm::rbtree_best_fit) {
                    fSegments.emplace(id, RBTreeBestFitSegment(open_only, std::string("fmq_" + fShmId + "_m_" + std::to_string(id)).c_str()));
                } else {
                    fSegments.emplace(id, SimpleSeqFitSegment(open_only, std::string("fmq_" + fShmId + "_m_" + std::to_string(id)).c_str()));
                }
            } catch (std::out_of_range& oor) {
                LOG(error) << "Could not get segment with id '" << id << "': " << oor.what();
            } catch (boost::interprocess::interprocess_exception& bie) {
                LOG(error) << "Could not get segment with id '" << id << "': " << bie.what();
            }
        }
    }

    boost::interprocess::managed_shared_memory::handle_t GetHandleFromAddress(const void* ptr, uint16_t segmentId) const
    {
        return boost::apply_visitor(SegmentHandleFromAddress(ptr), fSegments.at(segmentId));
    }
    void* GetAddressFromHandle(const boost::interprocess::managed_shared_memory::handle_t handle, uint16_t segmentId) const
    {
        return boost::apply_visitor(SegmentAddressFromHandle(handle), fSegments.at(segmentId));
    }

    char* Allocate(const size_t size, size_t alignment = 0)
    {
        char* ptr = nullptr;
        // tools::RateLimiter rateLimiter(20);

        while (ptr == nullptr) {
            try {
                // boost::interprocess::managed_shared_memory::size_type actualSize = size;
                // char* hint = 0; // unused for boost::interprocess::allocate_new
                // ptr = fSegments.at(fSegmentId).allocation_command<char>(boost::interprocess::allocate_new, size, actualSize, hint);
                size_t segmentSize = boost::apply_visitor(SegmentSize(), fSegments.at(fSegmentId));
                if (size > segmentSize) {
                    throw MessageBadAlloc(tools::ToString("Requested message size (", size, ") exceeds segment size (", segmentSize, ")"));
                }
                if (alignment == 0) {
                    ptr = reinterpret_cast<char*>(boost::apply_visitor(SegmentAllocate{size}, fSegments.at(fSegmentId)));
                } else {
                    ptr = reinterpret_cast<char*>(boost::apply_visitor(SegmentAllocateAligned{size, alignment}, fSegments.at(fSegmentId)));
                }
            } catch (boost::interprocess::bad_alloc& ba) {
                // LOG(warn) << "Shared memory full...";
                if (ThrowingOnBadAlloc()) {
                    throw MessageBadAlloc(tools::ToString("shmem: could not create a message of size ", size, ", alignment: ", (alignment != 0) ? std::to_string(alignment) : "default", ", free memory: ", boost::apply_visitor(SegmentFreeMemory(), fSegments.at(fSegmentId))));
                }
                // rateLimiter.maybe_sleep();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                if (Interrupted()) {
                    return ptr;
                } else {
                    continue;
                }
            }
#ifdef FAIRMQ_DEBUG_MODE
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);
            IncrementShmMsgCounter(fSegmentId);
            if (fMsgDebug->count(fSegmentId) == 0) {
                (*fMsgDebug).emplace(fSegmentId, fShmVoidAlloc);
            }
            (*fMsgDebug).at(fSegmentId).emplace(
                static_cast<size_t>(GetHandleFromAddress(ptr, fSegmentId)),
                MsgDebug(getpid(), size, std::chrono::system_clock::now().time_since_epoch().count())
            );
#endif
        }

        return ptr;
    }

    void Deallocate(boost::interprocess::managed_shared_memory::handle_t handle, uint16_t segmentId)
    {
        boost::apply_visitor(SegmentDeallocate(GetAddressFromHandle(handle, segmentId)), fSegments.at(segmentId));
#ifdef FAIRMQ_DEBUG_MODE
        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);
        DecrementShmMsgCounter(segmentId);
        try {
            (*fMsgDebug).at(segmentId).erase(handle);
        } catch(const std::out_of_range& oor) {
            LOG(debug) << "could not locate debug container for " << segmentId << ": " << oor.what();
        }
#endif
    }

    char* ShrinkInPlace(size_t newSize, char* localPtr, uint16_t segmentId)
    {
        return boost::apply_visitor(SegmentBufferShrink(newSize, localPtr), fSegments.at(segmentId));
    }

    uint16_t GetSegmentId() const { return fSegmentId; }

    void CleanupIfLast()
    {
        using namespace boost::interprocess;

        bool lastRemoved = false;
        try {
            boost::interprocess::scoped_lock<named_mutex> lock(fShmMtx);

            (fDeviceCounter->fCount)--;

            if (fDeviceCounter->fCount == 0) {
                LOG(debug) << "Last segment user, " << (fNoCleanup ? "skipping removal (--shm-no-cleanup is true)." : "removing segment.");
                lastRemoved = true;
            } else {
                LOG(debug) << "Other segment users present (" << fDeviceCounter->fCount << "), skipping removal.";
            }
        } catch (interprocess_exception& e) {
            LOG(error) << "Manager could not acquire lock: " << e.what();
        }

        if (lastRemoved && !fNoCleanup) {
            Monitor::Cleanup(ShmId{fShmId});
        }
    }

    ~Manager()
    {
        fRegionsGen += 1; // signal TL cache invalidation
        UnsubscribeFromRegionEvents();

        StopHeartbeats();

        CleanupIfLast();
    }

  private:
    uint64_t fShmId64;
    std::string fShmId;
    uint16_t fSegmentId;
    std::string fDeviceId;
    std::unordered_map<uint16_t, boost::variant<RBTreeBestFitSegment, SimpleSeqFitSegment>> fSegments;
    boost::interprocess::managed_shared_memory fManagementSegment;
    VoidAlloc fShmVoidAlloc;
    boost::interprocess::named_mutex fShmMtx;

    boost::interprocess::named_condition fRegionEventsShmCV;
    std::thread fRegionEventThread;
    bool fRegionEventsSubscriptionActive;
    std::function<void(fair::mq::RegionInfo)> fRegionEventCallback;
    std::map<std::pair<uint16_t, bool>, RegionEvent> fObservedRegionEvents; // pair: <region id, managed>
    uint64_t fNumObservedEvents;

    DeviceCounter* fDeviceCounter;
    EventCounter* fEventCounter;
    Uint16SegmentInfoHashMap* fShmSegments;
    Uint16RegionInfoHashMap* fShmRegions;
    std::unordered_map<uint16_t, std::unique_ptr<Region>> fRegions;
    // make sure this is alone in the cache line: mostly read
    alignas(128) inline static std::atomic<unsigned long> fRegionsGen = 0ul;
    inline static thread_local struct ManagerTLCache {
        unsigned long fRegionsTLCacheGen;
        std::vector<std::tuple<Region*, uint16_t, uint64_t>> fRegionsTLCache;
    } fTlRegionCache;

    std::atomic<bool> fInterrupted;
#ifdef FAIRMQ_DEBUG_MODE
    // make sure the counters are not thrashing the cache line between threads doing creation and deallocation
    Uint16MsgDebugMapHashMap* fMsgDebug;
    Uint16MsgCounterHashMap* fShmMsgCounters;
    alignas(128) std::atomic_uint64_t fMsgCounterNew;
    alignas(128) std::atomic_uint64_t fMsgCounterDelete;
#endif

    std::thread fHeartbeatThread;
    bool fBeatTheHeart;
    std::mutex fHeartbeatsMtx;
    std::condition_variable fHeartbeatsCV;

    bool fThrowOnBadAlloc;
    bool fNoCleanup;
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_MANAGER_H_ */

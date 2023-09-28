/********************************************************************************
 * Copyright (C) 2014-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SHMEM_MANAGER_H_
#define FAIR_MQ_SHMEM_MANAGER_H_

#include "Common.h"
#include "Monitor.h"
#include "UnmanagedRegion.h"
#include <fairmq/Message.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/tools/Strings.h>
#include <fairmq/Transports.h>

#include <fairlogger/Logger.h>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include <algorithm> // max
#include <chrono>
#include <condition_variable>
#include <cstddef> // max_align_t, std::size_t
#include <cstdlib> // getenv
#include <cstring> // memcpy
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
#include <variant>
#include <vector>

#include <unistd.h> // getuid
#include <sys/types.h> // getuid
#include <sys/mman.h> // mlock

namespace fair::mq::shmem
{

class Manager
{
  public:
    Manager(const std::string& sessionName, size_t size, const ProgOptions* config)
        : fShmId64(config ? config->GetProperty<uint64_t>("shmid", makeShmIdUint64(sessionName)) : makeShmIdUint64(sessionName))
        , fShmId(makeShmIdStr(fShmId64))
        , fSegmentId(config ? config->GetProperty<uint16_t>("shm-segment-id", 0) : 0)
        , fManagementSegment(boost::interprocess::open_or_create, MakeShmName(fShmId, "mng").c_str(), kManagementSegmentSize)
        , fShmVoidAlloc(fManagementSegment.get_segment_manager())
        , fShmMtx(fManagementSegment.find_or_construct<boost::interprocess::interprocess_mutex>(boost::interprocess::unique_instance)())
        , fNumObservedEvents(0)
        , fDeviceCounter(nullptr)
        , fEventCounter(nullptr)
        , fShmSegments(nullptr)
        , fShmRegions(nullptr)
#ifdef FAIRMQ_DEBUG_MODE
        , fMsgDebug(nullptr)
        , fShmMsgCounters(nullptr)
        , fMsgCounterNew(0)
        , fMsgCounterDelete(0)
#endif
        , fBeatTheHeart(true)
        , fRegionEventsSubscriptionActive(false)
        , fInterrupted(false)
        , fBadAllocMaxAttempts(1)
        , fBadAllocAttemptIntervalInMs(config ? config->GetProperty<int>("bad-alloc-attempt-interval", 50) : 50)
        , fNoCleanup(config ? config->GetProperty<bool>("shm-no-cleanup", false) : false)
        , fMetadataMsgSize(config ? config->GetProperty<std::size_t>("shm-metadata-msg-size", 0) : 0)
    {
        using namespace boost::interprocess;

        LOG(debug) << "Generated shmid '" << fShmId << "' out of session id '" << sessionName << "'.";

        if (config) {
            // if 'shm-throw-bad-alloc' is explicitly set to false (true is default), ignore other settings
            if (config->Count("shm-throw-bad-alloc") && config->GetProperty<bool>("shm-throw-bad-alloc") == false) {
                fBadAllocMaxAttempts = -1;
            } else if (config->Count("bad-alloc-max-attempts")) {
                // if 'bad-alloc-max-attempts' is provided, use it
                // this can override the default 'shm-throw-bad-alloc'==true if the value of 'bad-alloc-max-attempts' is -1
                fBadAllocMaxAttempts = config->GetProperty<int>("bad-alloc-max-attempts");
            }
            // otherwise leave fBadAllocMaxAttempts at 1 (the original default, set in the initializer list)
        }

        bool mlockSegment = false;
        bool mlockSegmentOnCreation = false;
        bool zeroSegment = false;
        bool zeroSegmentOnCreation = false;
        bool autolaunchMonitor = false;
        std::string allocationAlgorithm("rbtree_best_fit");
        if (config) {
            mlockSegment = config->GetProperty<bool>("shm-mlock-segment", mlockSegment);
            mlockSegmentOnCreation = config->GetProperty<bool>("shm-mlock-segment-on-creation", mlockSegmentOnCreation);
            zeroSegment = config->GetProperty<bool>("shm-zero-segment", zeroSegment);
            zeroSegmentOnCreation = config->GetProperty<bool>("shm-zero-segment-on-creation", zeroSegmentOnCreation);
            autolaunchMonitor = config->GetProperty<bool>("shm-monitor", autolaunchMonitor);
            allocationAlgorithm = config->GetProperty<std::string>("shm-allocation", allocationAlgorithm);
        } else {
            LOG(debug) << "ProgOptions not available! Using defaults.";
        }

        if (autolaunchMonitor) {
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*fShmMtx);
            StartMonitor(fShmId);
        }

        fHeartbeatThread = std::thread(&Manager::Heartbeats, this);

        try {
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*fShmMtx);

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
                LOG(trace) << "event counter found: " << fEventCounter->fCount;
            } else {
                LOG(trace) << "no event counter found, creating one and initializing with 0";
                fEventCounter = fManagementSegment.construct<EventCounter>(unique_instance)(0);
                LOG(trace) << "initialized event counter with: " << fEventCounter->fCount;
            }

            fDeviceCounter = fManagementSegment.find<DeviceCounter>(unique_instance).first;
            if (fDeviceCounter) {
                LOG(trace) << "device counter found, with value of " << fDeviceCounter->fCount << ". incrementing.";
                (fDeviceCounter->fCount)++;
                LOG(trace) << "incremented device counter, now: " << fDeviceCounter->fCount;
            } else {
                LOG(trace) << "no device counter found, creating one and initializing with 1";
                fDeviceCounter = fManagementSegment.construct<DeviceCounter>(unique_instance)(1);
                LOG(trace) << "initialized device counter with: " << fDeviceCounter->fCount;
            }

            fShmSegments = fManagementSegment.find_or_construct<Uint16SegmentInfoHashMap>(unique_instance)(fShmVoidAlloc);
            fShmRegions = fManagementSegment.find_or_construct<Uint16RegionInfoHashMap>(unique_instance)(fShmVoidAlloc);

            bool createdSegment = false;

            try {
                std::string segmentName = MakeShmName(fShmId, "m", fSegmentId);
                auto it = fShmSegments->find(fSegmentId);
                if (it == fShmSegments->end()) {
                    // no segment with given id exists, creating
                    if (allocationAlgorithm == "rbtree_best_fit") {
                        fSegments.emplace(fSegmentId, RBTreeBestFitSegment(open_or_create, segmentName.c_str(), size));
                        fShmSegments->emplace(fSegmentId, AllocationAlgorithm::rbtree_best_fit);
                    } else if (allocationAlgorithm == "simple_seq_fit") {
                        fSegments.emplace(fSegmentId, SimpleSeqFitSegment(open_or_create, segmentName.c_str(), size));
                        fShmSegments->emplace(fSegmentId, AllocationAlgorithm::simple_seq_fit);
                    }
                    if (mlockSegmentOnCreation) {
                        MlockSegment(fSegmentId);
                    }
                    if (zeroSegmentOnCreation) {
                        ZeroSegment(fSegmentId);
                    }
                    createdSegment = true;
                } else {
                    // found segment with the given id, opening
                    if (it->second.fAllocationAlgorithm == AllocationAlgorithm::rbtree_best_fit) {
                        fSegments.emplace(fSegmentId, RBTreeBestFitSegment(open_or_create, segmentName.c_str(), size));
                        if (allocationAlgorithm != "rbtree_best_fit") {
                            LOG(warn) << "Allocation algorithm of the opened segment is rbtree_best_fit, but requested is " << allocationAlgorithm << ". Ignoring requested setting.";
                            allocationAlgorithm = "rbtree_best_fit";
                        }
                    } else {
                        fSegments.emplace(fSegmentId, SimpleSeqFitSegment(open_or_create, segmentName.c_str(), size));
                        if (allocationAlgorithm != "simple_seq_fit") {
                            LOG(warn) << "Allocation algorithm of the opened segment is simple_seq_fit, but requested is " << allocationAlgorithm << ". Ignoring requested setting.";
                            allocationAlgorithm = "simple_seq_fit";
                        }
                    }
                }
                LOG(debug) << (createdSegment ? "Created" : "Opened") << " managed shared memory segment " << "fmq_" << fShmId << "_m_" << fSegmentId
                    << ". Size: " << std::visit([](auto& s) { return s.get_size(); }, fSegments.at(fSegmentId)) << " bytes."
                    << " Available: " << std::visit([](auto& s) { return s.get_free_memory(); }, fSegments.at(fSegmentId)) << " bytes."
                    << " Allocation algorithm: " << allocationAlgorithm;
            } catch (interprocess_exception& bie) {
                LOG(error) << "Failed to create/open shared memory segment '" << "fmq_" << fShmId << "_m_" << fSegmentId << "': " << bie.what();
                throw TransportError(tools::ToString("Failed to create/open shared memory segment '", "fmq_", fShmId, "_m_", fSegmentId, "': ", bie.what()));
            }

            if (mlockSegment) {
                MlockSegment(fSegmentId);
            }
            if (zeroSegment) {
                ZeroSegment(fSegmentId);
            }

            if (createdSegment) {
                (fEventCounter->fCount)++;
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
    Manager(Manager&&) = delete;
    Manager& operator=(const Manager&) = delete;
    Manager& operator=(Manager&&) = delete;

    void ZeroSegment(uint16_t id)
    {
        LOG(debug) << "Zeroing the managed segment free memory...";
        std::visit([](auto& s) { return s.zero_free_memory(); }, fSegments.at(id));
        LOG(debug) << "Successfully zeroed the managed segment free memory.";
    }

    void MlockSegment(uint16_t id)
    {
        LOG(debug) << "Locking the managed segment memory pages...";
        if (mlock(
                std::visit([](auto& s) { return s.get_address(); }, fSegments.at(id)),
                std::visit([](auto& s) { return s.get_size(); }, fSegments.at(id))) == -1) {
            LOG(error) << "Could not lock the managed segment memory. Code: " << errno << ", reason: " << strerror(errno);
            throw TransportError(tools::ToString("Could not lock the managed segment memory: ", strerror(errno)));
        }
        LOG(debug) << "Successfully locked the managed segment memory pages.";
    }

  private:
    static bool SpawnShmMonitor(const std::string& id);

  public:
    static void StartMonitor(const std::string& id)
    {
        using namespace boost::interprocess;
        try {
            named_mutex monitorStatus(open_only, MakeShmName(id, "ms").c_str());
            LOG(debug) << "Found fairmq-shmmonitor for shared memory id " << id;
        } catch (interprocess_exception&) {
            LOG(debug) << "no fairmq-shmmonitor found for shared memory id " << id << ", starting...";

            if (SpawnShmMonitor(id)) {
                int numTries = 0;
                do {
                    try {
                        named_mutex monitorStatus(open_only, MakeShmName(id, "ms").c_str());
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

    std::pair<UnmanagedRegion*, uint16_t> CreateRegion(size_t size,
                                                       RegionCallback callback,
                                                       RegionBulkCallback bulkCallback,
                                                       RegionConfig cfg)
    {
        using namespace boost::interprocess;
        try {
            std::pair<UnmanagedRegion*, uint16_t> result;

            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> shmLock(*fShmMtx);

                if (!cfg.id.has_value()) {
                    RegionCounter* rc = fManagementSegment.find<RegionCounter>(unique_instance).first;

                    if (rc) {
                        LOG(trace) << "region counter found, with value of " << rc->fCount << ". incrementing.";
                        (rc->fCount)++;
                        LOG(trace) << "incremented region counter, now: " << rc->fCount;
                    } else {
                        LOG(trace) << "no region counter found, creating one and initializing with 1024";
                        rc = fManagementSegment.construct<RegionCounter>(unique_instance)(1024);
                        LOG(trace) << "initialized region counter with: " << rc->fCount;
                    }

                    cfg.id = rc->fCount;
                }

                const uint16_t id = cfg.id.value();
                const uint64_t rcSegmentSize = cfg.rcSegmentSize;

                std::lock_guard<std::mutex> lock(fLocalRegionsMtx);

                UnmanagedRegion* region = nullptr;

                auto it = fRegions.find(id);
                if (it != fRegions.end()) {
                    region = it->second.get();
                    if (region->fControlling) {
                        LOG(error) << "Unmanaged Region with id " << id << " already exists. Only unique IDs per session are allowed.";
                        throw TransportError(tools::ToString("Unmanaged Region with id ", id, " already exists. Only unique IDs per session are allowed."));
                    }

                    LOG(debug) << "Unmanaged region (view) already present, promoting to controller";
                    region->BecomeController(cfg);
                } else {
                    auto res = fRegions.emplace(id, std::make_unique<UnmanagedRegion>(fShmId, size, true, cfg));
                    region = res.first->second.get();
                }
                // LOG(debug) << "Created region with id '" << id << "', path: '" << cfg.path << "', flags: '" << cfg.creationFlags << "'";

                // start ack receiver only if a callback has been provided.
                if (callback || bulkCallback) {
                    region->SetCallbacks(callback, bulkCallback);
                    region->InitializeRefCountSegment(rcSegmentSize);
                    region->InitializeQueues();
                    region->StartAckSender();
                    region->StartAckReceiver();
                }
                result.first = region;
                result.second = id;
            }
            fRegionsGen += 1; // signal TL cache invalidation

            return result;
        } catch (interprocess_exception& e) {
            LOG(error) << "cannot create region. Already created/not cleaned up?";
            LOG(error) << e.what();
            throw;
        }
    }

    UnmanagedRegion* GetRegionFromCache(uint16_t id)
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

        // slow path: check invalidation
        if (lTlCacheGen != fRegionsGen) {
            fTlRegionCache.fRegionsTLCache.clear();
        }

        auto* lRegion = GetRegion(id);
        fTlRegionCache.fRegionsTLCache.emplace_back(std::make_tuple(lRegion, id, fShmId64));
        fTlRegionCache.fRegionsTLCacheGen = fRegionsGen;
        return lRegion;
    }

    UnmanagedRegion* GetRegion(uint16_t id)
    {
        std::lock_guard<std::mutex> lock(fLocalRegionsMtx);
        auto it = fRegions.find(id);
        if (it != fRegions.end()) {
            return it->second.get();
        }

        try {
            RegionConfig cfg;
            const uint64_t rcSegmentSize = cfg.rcSegmentSize;
            // get region info
            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> shmLock(*fShmMtx);
                RegionInfo regionInfo = fShmRegions->at(id);
                cfg.id = id;
                cfg.creationFlags = regionInfo.fCreationFlags;
                cfg.path = regionInfo.fPath.c_str();
            }
            // LOG(debug) << "Located remote region with id '" << id << "', path: '" << cfg.path << "', flags: '" << cfg.creationFlags << "'";

            auto r = fRegions.emplace(id, std::make_unique<UnmanagedRegion>(fShmId, 0, false, std::move(cfg)));
            r.first->second->InitializeRefCountSegment(rcSegmentSize);
            r.first->second->InitializeQueues();
            r.first->second->StartAckSender();
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

    void RemoveRegion(uint16_t id)
    {
        try {
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> shmLock(*fShmMtx);
            std::lock_guard<std::mutex> lock(fLocalRegionsMtx);
            fRegions.at(id)->StopAcks();
            {
                if (fRegions.at(id)->RemoveOnDestruction()) {
                    fShmRegions->at(id).fDestroyed = true;
                    (fEventCounter->fCount)++;
                }
                fRegions.erase(id);
            }
        } catch (std::out_of_range& oor) {
            LOG(debug) << "RemoveRegion() could not locate region with id '" << id << "'";
        }
        fRegionsGen += 1; // signal TL cache invalidation
    }

    std::vector<fair::mq::RegionInfo> GetRegionInfo()
    {
        std::vector<fair::mq::RegionInfo> result;
        std::map<uint64_t, RegionConfig> regionCfgs;

        {
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> shmLock(*fShmMtx);

            for (const auto& [segmentId, segmentInfo] : *fShmSegments) {
                // make sure any segments in the session are found
                GetSegment(segmentId);
                try {
                    fair::mq::RegionInfo info;
                    info.managed = true;
                    info.id = segmentId;
                    info.event = RegionEvent::created;
                    info.ptr = std::visit([](auto& s) { return s.get_address(); }, fSegments.at(segmentId));
                    info.size = std::visit([](auto& s) { return s.get_size(); }, fSegments.at(segmentId));
                    result.push_back(info);
                } catch (const std::out_of_range& oor) {
                    LOG(error) << "could not find segment with id " << segmentId;
                    LOG(error) << oor.what();
                }
            }

            for (const auto& [regionId, regionInfo] : *fShmRegions) {
                fair::mq::RegionInfo info;
                info.managed = false;
                info.id = regionId;
                info.flags = regionInfo.fUserFlags;
                info.event = regionInfo.fDestroyed ? RegionEvent::destroyed : RegionEvent::created;
                if (info.event == RegionEvent::created) {
                    RegionConfig cfg;
                    cfg.id = info.id;
                    cfg.creationFlags = regionInfo.fCreationFlags;
                    cfg.path = regionInfo.fPath.c_str();
                    regionCfgs.emplace(info.id, cfg);
                    // fill the ptr+size info after shmLock is released, to avoid constructing local region under it
                } else {
                    info.ptr = nullptr;
                    info.size = 0;
                }
                result.push_back(info);
            }
        }

        // do another iteration outside of shm lock, to fill ptr+size of unmanaged regions
        for (auto& info : result) {
            if (!info.managed && info.event == RegionEvent::created) {
                auto cfgIt = regionCfgs.find(info.id);
                if (cfgIt != regionCfgs.end()) {
                    UnmanagedRegion* region = nullptr;
                    std::lock_guard<std::mutex> lock(fLocalRegionsMtx);
                    auto it = fRegions.find(info.id);
                    if (it != fRegions.end()) {
                        region = it->second.get();
                    } else {
                        const uint64_t rcSegmentSize = cfgIt->second.rcSegmentSize;
                        auto r = fRegions.emplace(cfgIt->first, std::make_unique<UnmanagedRegion>(fShmId, 0, false, cfgIt->second));
                        region = r.first->second.get();
                        region->InitializeRefCountSegment(rcSegmentSize);
                        region->InitializeQueues();
                        region->StartAckSender();
                    }

                    info.ptr = region->GetData();
                    info.size = region->GetSize();
                } else {
                    info.ptr = nullptr;
                    info.size = 0;
                }
            }
        }

        return result;
    }

    void SubscribeToRegionEvents(RegionEventCallback callback)
    {
        if (fRegionEventThread.joinable()) {
            LOG(debug) << "Already subscribed. Overwriting previous subscription.";
            std::unique_lock<std::mutex> lock(fRegionEventsMtx);
            fRegionEventsSubscriptionActive = false;
            lock.unlock();
            fRegionEventsCV.notify_one();
            fRegionEventThread.join();
        }
        std::lock_guard<std::mutex> lock(fRegionEventsMtx);
        fRegionEventCallback = callback;
        fRegionEventsSubscriptionActive = true;
        fRegionEventThread = std::thread(&Manager::RegionEventsSubscription, this);
    }

    bool SubscribedToRegionEvents() { return fRegionEventThread.joinable(); }

    void UnsubscribeFromRegionEvents()
    {
        if (fRegionEventThread.joinable()) {
            std::unique_lock<std::mutex> lock(fRegionEventsMtx);
            fRegionEventsSubscriptionActive = false;
            lock.unlock();
            fRegionEventsCV.notify_one();
            fRegionEventThread.join();
            lock.lock();
            fRegionEventCallback = nullptr;
        }
    }

    void RegionEventsSubscription()
    {
        std::unique_lock<std::mutex> lock(fRegionEventsMtx);

        while (fRegionEventsSubscriptionActive) {
            if (fNumObservedEvents != fEventCounter->fCount) {
                auto infos = GetRegionInfo();

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
            }
            // TODO: do better than polling here, without adding too much shmem contention
            fRegionEventsCV.wait_for(lock, std::chrono::milliseconds(50), [&] { return !fRegionEventsSubscriptionActive; });
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
                    fSegments.emplace(id, RBTreeBestFitSegment(open_only, MakeShmName(fShmId, "m", id).c_str()));
                } else {
                    fSegments.emplace(id, SimpleSeqFitSegment(open_only, MakeShmName(fShmId, "m", id).c_str()));
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
        return std::visit([ptr](auto& s) { return s.get_handle_from_address(ptr); }, fSegments.at(segmentId));
    }
    char* GetAddressFromHandle(const boost::interprocess::managed_shared_memory::handle_t handle, uint16_t segmentId) const
    {
        return std::visit([handle](auto& s) { return reinterpret_cast<char*>(s.get_address_from_handle(handle)); }, fSegments.at(segmentId));
    }

    char* Allocate(size_t size, size_t alignment = 0)
    {
        alignment = std::max(alignment, alignof(std::max_align_t));

        char* ptr = nullptr;
        int numAttempts = 0;
        size_t fullSize = ShmHeader::FullSize(size, alignment);

        while (!ptr) {
            try {
                size_t segmentSize = std::visit([](auto& s) { return s.get_size(); }, fSegments.at(fSegmentId));
                if (fullSize > segmentSize) {
                    throw MessageBadAlloc(tools::ToString("Requested message size (", fullSize, ") exceeds segment size (", segmentSize, ")"));
                }

                ptr = std::visit([fullSize](auto& s) { return reinterpret_cast<char*>(s.allocate(fullSize)); }, fSegments.at(fSegmentId));
                ShmHeader::Construct(ptr, alignment);
            } catch (boost::interprocess::bad_alloc& ba) {
                // LOG(warn) << "Shared memory full...";
                if (fBadAllocMaxAttempts >= 0 && ++numAttempts >= fBadAllocMaxAttempts) {
                    throw MessageBadAlloc(tools::ToString("shmem: could not create a message of size ", size,
                        ", alignment: ", (alignment != 0) ? std::to_string(alignment) : "default",
                        ", free memory: ", std::visit([](auto& s) { return s.get_free_memory(); }, fSegments.at(fSegmentId))));
                }
                if (numAttempts == 1 && fBadAllocMaxAttempts > 1) {
                    LOG(warn) << tools::ToString("shmem: could not create a message of size ", size,
                        ", alignment: ", (alignment != 0) ? std::to_string(alignment) : "default",
                        ", free memory: ", std::visit([](auto& s) { return s.get_free_memory(); }, fSegments.at(fSegmentId)),
                        ". Will try ", (fBadAllocMaxAttempts > 1 ? (std::to_string(fBadAllocMaxAttempts - 1)) + " more times" : " until success"),
                        ", in ", fBadAllocAttemptIntervalInMs, "ms intervals");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(fBadAllocAttemptIntervalInMs));
                if (Interrupted()) {
                    throw MessageBadAlloc(tools::ToString("shmem: could not create a message of size ", size,
                        ", alignment: ", (alignment != 0) ? std::to_string(alignment) : "default",
                        ", free memory: ", std::visit([](auto& s) { return s.get_free_memory(); }, fSegments.at(fSegmentId))));
                } else {
                    continue;
                }
            }
#ifdef FAIRMQ_DEBUG_MODE
            boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*fShmMtx);
            IncrementShmMsgCounter(fSegmentId);
            if (fMsgDebug->count(fSegmentId) == 0) {
                fMsgDebug->emplace(fSegmentId, fShmVoidAlloc);
            }
            fMsgDebug->at(fSegmentId).emplace(
                static_cast<size_t>(GetHandleFromAddress(ShmHeader::UserPtr(ptr), fSegmentId)),
                MsgDebug(getpid(), size, std::chrono::system_clock::now().time_since_epoch().count())
            );
#endif
        }

        return ptr;
    }

    void Deallocate(boost::interprocess::managed_shared_memory::handle_t handle, uint16_t segmentId)
    {
        char* ptr = GetAddressFromHandle(handle, segmentId);
#ifdef FAIRMQ_DEBUG_MODE
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(*fShmMtx);
        DecrementShmMsgCounter(segmentId);
        try {
            fMsgDebug->at(segmentId).erase(GetHandleFromAddress(ShmHeader::UserPtr(ptr), fSegmentId));
        } catch (const std::out_of_range& oor) {
            LOG(debug) << "could not locate debug container for " << segmentId << ": " << oor.what();
        }
#endif
        ShmHeader::Destruct(ptr);
        std::visit([ptr](auto& s) { s.deallocate(ptr); }, fSegments.at(segmentId));
    }

    char* ShrinkInPlace(size_t newSize, char* localPtr, uint16_t segmentId)
    {
        return std::visit(SegmentBufferShrink(newSize, localPtr), fSegments.at(segmentId));
    }

    uint16_t GetSegmentId() const { return fSegmentId; }

    void CleanupIfLast()
    {
        using namespace boost::interprocess;

        bool lastRemoved = false;
        try {
            boost::interprocess::scoped_lock<interprocess_mutex> lock(*fShmMtx);

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

        if (lastRemoved) {
            if (!fNoCleanup) {
                Monitor::Cleanup(ShmId{fShmId});
            }
        }
    }

    auto GetMetadataMsgSize() const noexcept { return fMetadataMsgSize; }

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
    std::unordered_map<uint16_t, std::variant<RBTreeBestFitSegment, SimpleSeqFitSegment>> fSegments; // TODO: refactor to use Segment class
    boost::interprocess::managed_shared_memory fManagementSegment; // TODO: refactor to use ManagementSegment class
    VoidAlloc fShmVoidAlloc;
    boost::interprocess::interprocess_mutex* fShmMtx;

    std::mutex fLocalRegionsMtx;
    std::mutex fRegionEventsMtx;
    std::condition_variable fRegionEventsCV;
    std::thread fRegionEventThread;
    std::function<void(fair::mq::RegionInfo)> fRegionEventCallback;
    std::map<std::pair<uint16_t, bool>, RegionEvent> fObservedRegionEvents; // pair: <region id, managed>
    uint64_t fNumObservedEvents;

    DeviceCounter* fDeviceCounter;
    EventCounter* fEventCounter;
    Uint16SegmentInfoHashMap* fShmSegments;
    Uint16RegionInfoHashMap* fShmRegions;
    std::unordered_map<uint16_t, std::unique_ptr<UnmanagedRegion>> fRegions;
    inline static std::atomic<unsigned long> fRegionsGen = 0ul;
    inline static thread_local struct ManagerTLCache {
        unsigned long fRegionsTLCacheGen;
        std::vector<std::tuple<UnmanagedRegion*, uint16_t, uint64_t>> fRegionsTLCache;
    } fTlRegionCache;

#ifdef FAIRMQ_DEBUG_MODE
    Uint16MsgDebugMapHashMap* fMsgDebug;
    Uint16MsgCounterHashMap* fShmMsgCounters;
    std::atomic_uint64_t fMsgCounterNew;
    std::atomic_uint64_t fMsgCounterDelete;
#endif

    std::thread fHeartbeatThread;
    std::mutex fHeartbeatsMtx;
    std::condition_variable fHeartbeatsCV;
    bool fBeatTheHeart;

    bool fRegionEventsSubscriptionActive;
    std::atomic<bool> fInterrupted;

    int fBadAllocMaxAttempts;
    int fBadAllocAttemptIntervalInMs;
    bool fNoCleanup;

    std::size_t fMetadataMsgSize;
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_MANAGER_H_ */

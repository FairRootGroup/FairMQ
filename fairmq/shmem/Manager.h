/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
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

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/variant.hpp>

#include <algorithm> // max
#include <condition_variable>
#include <cstddef> // max_align_t
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
#include <vector>

#include <unistd.h> // getuid
#include <sys/types.h> // getuid
#include <sys/mman.h> // mlock

namespace fair::mq::shmem
{

// ShmHeader stores user buffer alignment and the reference count in the following structure:
// [HdrOffset(uint16_t)][Hdr alignment][Hdr][user buffer alignment][user buffer]
// The alignment of Hdr depends on the alignment of std::atomic and is stored in the first entry
struct ShmHeader
{
    struct Hdr
    {
        uint16_t userOffset;
        std::atomic<uint16_t> refCount;
    };

    static Hdr* HdrPtr(char* ptr)
    {
        // [HdrOffset(uint16_t)][Hdr alignment][Hdr][user buffer alignment][user buffer]
        //                                     ^
        return reinterpret_cast<Hdr*>(ptr + sizeof(uint16_t) + *(reinterpret_cast<uint16_t*>(ptr)));
    }

    static uint16_t HdrPartSize() // [HdrOffset(uint16_t)][Hdr alignment][Hdr]
    {
        // [HdrOffset(uint16_t)][Hdr alignment][Hdr][user buffer alignment][user buffer]
        // <--------------------------------------->
        return sizeof(uint16_t) + alignof(Hdr) + sizeof(Hdr);
    }

    static std::atomic<uint16_t>& RefCountPtr(char* ptr)
    {
        // get the ref count ptr from the Hdr
        return HdrPtr(ptr)->refCount;
    }

    static uint16_t UserOffset(char* ptr)
    {
        return HdrPartSize() + HdrPtr(ptr)->userOffset;
    }

    static char* UserPtr(char* ptr)
    {
        // [HdrOffset(uint16_t)][Hdr alignment][Hdr][user buffer alignment][user buffer]
        //                                                                 ^
        return ptr + HdrPartSize() + HdrPtr(ptr)->userOffset;
    }

    static uint16_t RefCount(char* ptr) { return RefCountPtr(ptr).load(); }
    static uint16_t IncrementRefCount(char* ptr) { return RefCountPtr(ptr).fetch_add(1); }
    static uint16_t DecrementRefCount(char* ptr) { return RefCountPtr(ptr).fetch_sub(1); }

    static size_t FullSize(size_t size, size_t alignment)
    {
        // [HdrOffset(uint16_t)][Hdr alignment][Hdr][user buffer alignment][user buffer]
        // <--------------------------------------------------------------------------->
        return HdrPartSize() + alignment + size;
    }

    static void Construct(char* ptr, size_t alignment)
    {
        // place the Hdr in the aligned location, fill it and store its offset to HdrOffset

        // the address alignment should be at least 2
        assert(reinterpret_cast<uintptr_t>(ptr) % 2 == 0);

        // offset to the beginning of the Hdr. store it in the beginning
        uint16_t hdrOffset = alignof(Hdr) - ((reinterpret_cast<uintptr_t>(ptr) + sizeof(uint16_t)) % alignof(Hdr));
        memcpy(ptr, &hdrOffset, sizeof(hdrOffset));

        // offset to the beginning of the user buffer, store in Hdr together with the ref count
        uint16_t userOffset = alignment - ((reinterpret_cast<uintptr_t>(ptr) + HdrPartSize()) % alignment);
        new(ptr + sizeof(uint16_t) + hdrOffset) Hdr{ userOffset, std::atomic<uint16_t>(1) };
    }

    static void Destruct(char* ptr) { RefCountPtr(ptr).~atomic(); }
};

class Manager
{
  public:
    Manager(const std::string& sessionName, size_t size, const ProgOptions* config)
        : fShmId64(config ? config->GetProperty<uint64_t>("shmid", makeShmIdUint64(sessionName)) : makeShmIdUint64(sessionName))
        , fShmId(makeShmIdStr(fShmId64))
        , fSegmentId(config ? config->GetProperty<uint16_t>("shm-segment-id", 0) : 0)
        , fManagementSegment(boost::interprocess::open_or_create, std::string("fmq_" + fShmId + "_mng").c_str(), kManagementSegmentSize)
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
                LOG(debug) << "event counter found: " << fEventCounter->fCount;
            } else {
                LOG(debug) << "no event counter found, creating one and initializing with 0";
                fEventCounter = fManagementSegment.construct<EventCounter>(unique_instance)(0);
                LOG(debug) << "initialized event counter with: " << fEventCounter->fCount;
            }

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

            fShmSegments = fManagementSegment.find_or_construct<Uint16SegmentInfoHashMap>(unique_instance)(fShmVoidAlloc);
            fShmRegions = fManagementSegment.find_or_construct<Uint16RegionInfoHashMap>(unique_instance)(fShmVoidAlloc);

            bool createdSegment = false;

            try {
                std::string segmentName("fmq_" + fShmId + "_m_" + std::to_string(fSegmentId));
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
                LOG(debug) << "Created/opened shared memory segment '" << "fmq_" << fShmId << "_m_" << fSegmentId << "'."
                << " Size: " << boost::apply_visitor(SegmentSize(), fSegments.at(fSegmentId)) << " bytes."
                << " Available: " << boost::apply_visitor(SegmentFreeMemory(), fSegments.at(fSegmentId)) << " bytes."
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
        boost::apply_visitor(SegmentMemoryZeroer(), fSegments.at(id));
        LOG(debug) << "Successfully zeroed the managed segment free memory.";
    }

    void MlockSegment(uint16_t id)
    {
        LOG(debug) << "Locking the managed segment memory pages...";
        if (mlock(boost::apply_visitor(SegmentAddress(), fSegments.at(id)), boost::apply_visitor(SegmentSize(), fSegments.at(id))) == -1) {
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

                UnmanagedRegion* region = nullptr;
                bool newRegionCreated = false;
                {
                    std::lock_guard<std::mutex> lock(fLocalRegionsMtx);
                    auto res = fRegions.emplace(id, std::make_unique<UnmanagedRegion>(fShmId, size, false, cfg));
                    newRegionCreated = res.second;
                    region = res.first->second.get();
                }
                // LOG(debug) << "Created region with id '" << id << "', path: '" << cfg.path << "', flags: '" << cfg.creationFlags << "'";

                if (!newRegionCreated) {
                    region->fRemote = false; // TODO: this should be more clear, refactor it.
                }

                // start ack receiver only if a callback has been provided.
                if (callback || bulkCallback) {
                    region->SetCallbacks(callback, bulkCallback);
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

    UnmanagedRegion* GetRegion(uint16_t id)
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

        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> shmLock(*fShmMtx);
        // slow path: check invalidation
        if (lTlCacheGen != fRegionsGen) {
            fTlRegionCache.fRegionsTLCache.clear();
        }

        std::lock_guard<std::mutex> lock(fLocalRegionsMtx);
        auto* lRegion = GetRegionUnsafe(id, shmLock);
        fTlRegionCache.fRegionsTLCache.emplace_back(std::make_tuple(lRegion, id, fShmId64));
        fTlRegionCache.fRegionsTLCacheGen = fRegionsGen;
        return lRegion;
    }

    UnmanagedRegion* GetRegionUnsafe(uint16_t id, boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex>& lockedShmLock)
    {
        // remote region could actually be a local one if a message originates from this device (has been sent out and returned)
        auto it = fRegions.find(id);
        if (it != fRegions.end()) {
            return it->second.get();
        } else {
            try {
                // get region info
                RegionInfo regionInfo = fShmRegions->at(id);
                // safe to unlock now - no shm container accessed after this
                lockedShmLock.unlock();
                RegionConfig cfg;
                cfg.id = id;
                cfg.creationFlags = regionInfo.fCreationFlags;
                cfg.path = regionInfo.fPath.c_str();
                // LOG(debug) << "Located remote region with id '" << id << "', path: '" << cfg.path << "', flags: '" << cfg.creationFlags << "'";

                auto r = fRegions.emplace(id, std::make_unique<UnmanagedRegion>(fShmId, 0, true, std::move(cfg)));
                r.first->second->InitializeQueues();
                r.first->second->StartAckSender();
                lockedShmLock.lock();
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

    void RemoveRegion(uint16_t id)
    {
        try {
            std::lock_guard<std::mutex> lock(fLocalRegionsMtx);
            fRegions.at(id)->StopAcks();
            {
                boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> shmLock(*fShmMtx);
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
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> shmLock(*fShmMtx);

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

        for (const auto& e : *fShmRegions) {
            fair::mq::RegionInfo info;
            info.managed = false;
            info.id = e.first;
            info.flags = e.second.fUserFlags;
            info.event = e.second.fDestroyed ? RegionEvent::destroyed : RegionEvent::created;
            if (info.event == RegionEvent::created) {
                auto region = GetRegionUnsafe(info.id, shmLock);
                if (region) {
                    info.ptr = region->GetData();
                    info.size = region->GetSize();
                } else {
                    throw std::runtime_error(tools::ToString("GetRegionInfo() could not get region with id '", info.id, "'"));
                }
            } else {
                info.ptr = nullptr;
                info.size = 0;
            }
            result.push_back(info);
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
    char* GetAddressFromHandle(const boost::interprocess::managed_shared_memory::handle_t handle, uint16_t segmentId) const
    {
        return boost::apply_visitor(SegmentAddressFromHandle(handle), fSegments.at(segmentId));
    }

    char* Allocate(size_t size, size_t alignment = 0)
    {
        alignment = std::max(alignment, alignof(std::max_align_t));

        char* ptr = nullptr;
        int numAttempts = 0;
        size_t fullSize = ShmHeader::FullSize(size, alignment);

        while (!ptr) {
            try {
                size_t segmentSize = boost::apply_visitor(SegmentSize(), fSegments.at(fSegmentId));
                if (fullSize > segmentSize) {
                    throw MessageBadAlloc(tools::ToString("Requested message size (", fullSize, ") exceeds segment size (", segmentSize, ")"));
                }

                ptr = boost::apply_visitor(SegmentAllocate{fullSize}, fSegments.at(fSegmentId));
                ShmHeader::Construct(ptr, alignment);
            } catch (boost::interprocess::bad_alloc& ba) {
                // LOG(warn) << "Shared memory full...";
                if (fBadAllocMaxAttempts >= 0 && ++numAttempts >= fBadAllocMaxAttempts) {
                    throw MessageBadAlloc(tools::ToString("shmem: could not create a message of size ", size, ", alignment: ", (alignment != 0) ? std::to_string(alignment) : "default", ", free memory: ", boost::apply_visitor(SegmentFreeMemory(), fSegments.at(fSegmentId))));
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(fBadAllocAttemptIntervalInMs));
                if (Interrupted()) {
                    throw MessageBadAlloc(tools::ToString("shmem: could not create a message of size ", size, ", alignment: ", (alignment != 0) ? std::to_string(alignment) : "default", ", free memory: ", boost::apply_visitor(SegmentFreeMemory(), fSegments.at(fSegmentId))));
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
        boost::apply_visitor(SegmentDeallocate(ptr), fSegments.at(segmentId));
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
    std::unordered_map<uint16_t, boost::variant<RBTreeBestFitSegment, SimpleSeqFitSegment>> fSegments; // TODO: refactor to use Segment class
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
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_MANAGER_H_ */

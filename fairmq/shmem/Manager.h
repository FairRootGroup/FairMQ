/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
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
#include "Region.h"
#include "Monitor.h"

#include <FairMQLogger.h>
#include <FairMQMessage.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/tools/CppSTL.h>
#include <fairmq/tools/Strings.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/process.hpp>
#include <boost/variant.hpp>

#include <cstdlib> // getenv
#include <condition_variable>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility> // pair
#include <vector>

#include <sys/mman.h> // mlock

namespace fair
{
namespace mq
{
namespace shmem
{

class Manager
{
  public:
    Manager(std::string shmId, std::string deviceId, size_t size, const ProgOptions* config)
        : fShmId(std::move(shmId))
        , fDeviceId(std::move(deviceId))
        , fSegments()
        , fManagementSegment(boost::interprocess::open_or_create, std::string("fmq_" + fShmId + "_mng").c_str(), 6553600)
        , fShmVoidAlloc(fManagementSegment.get_segment_manager())
        , fShmMtx(boost::interprocess::open_or_create, std::string("fmq_" + fShmId + "_mtx").c_str())
        , fRegionEventsCV(boost::interprocess::open_or_create, std::string("fmq_" + fShmId + "_cv").c_str())
        , fRegionEventsSubscriptionActive(false)
        , fDeviceCounter(nullptr)
        , fShmSegments(nullptr)
        , fShmRegions(nullptr)
        , fInterrupted(false)
        , fMsgCounter(0)
#ifdef FAIRMQ_DEBUG_MODE
        , fMsgDebug(nullptr)
        , fShmMsgCounter(nullptr)
#endif
        , fHeartbeatThread()
        , fSendHeartbeats(true)
        , fThrowOnBadAlloc(true)
    {
        using namespace boost::interprocess;

        bool mlockSegment = false;
        bool zeroSegment = false;
        bool autolaunchMonitor = false;
        std::string allocationAlgorithm("rbtree_best_fit");
        if (config) {
            mlockSegment = config->GetProperty<bool>("shm-mlock-segment", mlockSegment);
            zeroSegment = config->GetProperty<bool>("shm-zero-segment", zeroSegment);
            autolaunchMonitor = config->GetProperty<bool>("shm-monitor", autolaunchMonitor);
            fThrowOnBadAlloc = config->GetProperty<bool>("shm-throw-bad-alloc", fThrowOnBadAlloc);
            allocationAlgorithm = config->GetProperty<std::string>("shm-allocation", allocationAlgorithm);
        } else {
            LOG(debug) << "ProgOptions not available! Using defaults.";
        }

        if (allocationAlgorithm != "rbtree_best_fit" && allocationAlgorithm != "simple_seq_fit") {
            LOG(error) << "Provided shared memory allocation algorithm '" << allocationAlgorithm << "' is not supported. Supported are 'rbtree_best_fit'/'simple_seq_fit'";
            throw SharedMemoryError(tools::ToString("Provided shared memory allocation algorithm '", allocationAlgorithm, "' is not supported. Supported are 'rbtree_best_fit'/'simple_seq_fit'"));
        }

        if (autolaunchMonitor) {
            StartMonitor(fShmId);
        }

        {
            std::stringstream ss;
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);

            fShmSegments = fManagementSegment.find_or_construct<Uint64SegmentInfoHashMap>(unique_instance)(fShmVoidAlloc);

            const uint64_t id = 0;

            try {
                auto it = fShmSegments->find(id);
                if (it == fShmSegments->end()) {
                    // no segment with given id exists, creating
                    if (allocationAlgorithm == "rbtree_best_fit") {
                        fSegments.emplace(id, RBTreeBestFitSegment(create_only, std::string("fmq_" + fShmId + "_main").c_str(), size));
                        fShmSegments->emplace(id, AllocationAlgorithm::rbtree_best_fit);
                    } else if (allocationAlgorithm == "simple_seq_fit") {
                        fSegments.emplace(id, SimpleSeqFitSegment(create_only, std::string("fmq_" + fShmId + "_main").c_str(), size));
                        fShmSegments->emplace(id, AllocationAlgorithm::simple_seq_fit);
                    }
                    ss << "Created ";
                } else {
                    // found segment with the given id, opening
                    if (it->second.fAllocationAlgorithm == AllocationAlgorithm::rbtree_best_fit) {
                        fSegments.emplace(id, RBTreeBestFitSegment(open_only, std::string("fmq_" + fShmId + "_main").c_str()));
                        if (allocationAlgorithm != "rbtree_best_fit") {
                            LOG(warn) << "Allocation algorithm of the opened segment is rbtree_best_fit, but requested is " << allocationAlgorithm << ". Ignoring requested setting.";
                            allocationAlgorithm = "rbtree_best_fit";
                        }
                    } else {
                        fSegments.emplace(id, SimpleSeqFitSegment(open_only, std::string("fmq_" + fShmId + "_main").c_str()));
                        if (allocationAlgorithm != "simple_seq_fit") {
                            LOG(warn) << "Allocation algorithm of the opened segment is simple_seq_fit, but requested is " << allocationAlgorithm << ". Ignoring requested setting.";
                            allocationAlgorithm = "simple_seq_fit";
                        }
                    }
                    ss << "Opened ";
                }
                ss << "shared memory segment '" << "fmq_" << fShmId << "_main_" << id << "'."
                << " Size: " << boost::apply_visitor(SegmentSize{}, fSegments.at(id)) << " bytes."
                << " Available: " << boost::apply_visitor(SegmentFreeMemory{}, fSegments.at(id)) << " bytes."
                << " Allocation algorithm: " << allocationAlgorithm;
                LOG(debug) << ss.str();
            } catch(interprocess_exception&) {
                LOG(error) << "something went wrong";
            }
        }

        if (mlockSegment) {
            LOG(debug) << "Locking the managed segment memory pages...";
            if (mlock(boost::apply_visitor(SegmentAddress{}, fSegments.at(0)), boost::apply_visitor(SegmentSize{}, fSegments.at(0))) == -1) {
                LOG(error) << "Could not lock the managed segment memory. Code: " << errno << ", reason: " << strerror(errno);
            }
            LOG(debug) << "Successfully locked the managed segment memory pages.";
        }
        if (zeroSegment) {
            LOG(debug) << "Zeroing the managed segment free memory...";
            boost::apply_visitor(SegmentMemoryZeroer{}, fSegments.at(0));
            LOG(debug) << "Successfully zeroed the managed segment free memory.";
        }

        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);

            fShmRegions = fManagementSegment.find_or_construct<Uint64RegionInfoHashMap>(unique_instance)(fShmVoidAlloc);

#ifdef FAIRMQ_DEBUG_MODE
            fMsgDebug = fManagementSegment.find_or_construct<SizetMsgDebugMap>(unique_instance)(fShmVoidAlloc);
#endif

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

#ifdef FAIRMQ_DEBUG_MODE
            fShmMsgCounter = fManagementSegment.find<MsgCounter>(unique_instance).first;

            if (fShmMsgCounter) {
                LOG(debug) << "message counter found, with value of " << fShmMsgCounter->fCount << ".";
            } else {
                LOG(debug) << "no message counter found, creating one and initializing with 0";
                fShmMsgCounter = fManagementSegment.construct<MsgCounter>(unique_instance)(0);
                LOG(debug) << "initialized message counter with: " << fShmMsgCounter->fCount;
            }
#endif
        }

        fHeartbeatThread = std::thread(&Manager::SendHeartbeats, this);
    }

    Manager() = delete;

    Manager(const Manager&) = delete;
    Manager operator=(const Manager&) = delete;

    static void StartMonitor(const std::string& id)
    {
        using namespace boost::interprocess;
        try {
            named_mutex monitorStatus(open_only, std::string("fmq_" + id + "_ms").c_str());
            LOG(debug) << "Found fairmq-shmmonitor for shared memory id " << id;
        } catch (interprocess_exception&) {
            LOG(debug) << "no fairmq-shmmonitor found for shared memory id " << id << ", starting...";
            auto env = boost::this_process::environment();

            std::vector<boost::filesystem::path> ownPath = boost::this_process::path();

            if (const char* fmqp = getenv("FAIRMQ_PATH")) {
                ownPath.insert(ownPath.begin(), boost::filesystem::path(fmqp));
            }

            boost::filesystem::path p = boost::process::search_path("fairmq-shmmonitor", ownPath);

            if (!p.empty()) {
                boost::process::spawn(p, "-x", "--shmid", id, "-d", "-t", "2000", env);
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
            } else {
                LOG(warn) << "could not find fairmq-shmmonitor in the path";
            }
        }
    }

    void Interrupt() { fInterrupted.store(true); }
    void Resume() { fInterrupted.store(false); }
    void Reset()
    {
        if (fMsgCounter.load() != 0) {
            LOG(error) << "Message counter during Reset expected to be 0, found: " << fMsgCounter.load();
            throw MessageError(tools::ToString("Message counter during Reset expected to be 0, found: ", fMsgCounter.load()));
        }
    }
    bool Interrupted() { return fInterrupted.load(); }

    std::pair<boost::interprocess::mapped_region*, uint64_t> CreateRegion(const size_t size,
                                                                          const int64_t userFlags,
                                                                          RegionCallback callback,
                                                                          RegionBulkCallback bulkCallback,
                                                                          const std::string& path = "",
                                                                          int flags = 0)
    {
        using namespace boost::interprocess;
        try {
            std::pair<mapped_region*, uint64_t> result;

            {
                uint64_t id = 0;
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

                // create region info
                fShmRegions->emplace(id, RegionInfo(path.c_str(), flags, userFlags, fShmVoidAlloc));

                auto r = fRegions.emplace(id, tools::make_unique<Region>(fShmId, id, size, false, callback, bulkCallback, path, flags));
                // LOG(debug) << "Created region with id '" << id << "', path: '" << path << "', flags: '" << flags << "'";

                r.first->second->StartReceivingAcks();
                result.first = &(r.first->second->fRegion);
                result.second = id;
            }
            fRegionEventsCV.notify_all();

            return result;

        } catch (interprocess_exception& e) {
            LOG(error) << "cannot create region. Already created/not cleaned up?";
            LOG(error) << e.what();
            throw;
        }
    }

    Region* GetRegion(const uint64_t id)
    {
        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);
        return GetRegionUnsafe(id);
    }

    Region* GetRegionUnsafe(const uint64_t id)
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

                auto r = fRegions.emplace(id, tools::make_unique<Region>(fShmId, id, 0, true, nullptr, nullptr, path, flags));
                return r.first->second.get();
            } catch (std::out_of_range& oor) {
                LOG(error) << "Could not get remote region with id '" << id << "'. Does the region creator run with the same session id?";
                LOG(error) << oor.what();
                return nullptr;
            } catch (boost::interprocess::interprocess_exception& e) {
                LOG(warn) << "Could not get remote region for id '" << id << "'";
                return nullptr;
            }
        }
    }

    void RemoveRegion(const uint64_t id)
    {
        fRegions.erase(id);
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);
            fShmRegions->at(id).fDestroyed = true;
        }
        fRegionEventsCV.notify_all();
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
                info.ptr = region->fRegion.get_address();
                info.size = region->fRegion.get_size();
            } else {
                info.ptr = nullptr;
                info.size = 0;
            }
            result.push_back(info);
        }

        for (const auto& e : *fShmSegments) {
            fair::mq::RegionInfo info;
            info.managed = true;
            info.id = e.first;
            info.event = RegionEvent::created;
            info.ptr = boost::apply_visitor(SegmentAddress{}, fSegments.at(e.first));
            info.size = boost::apply_visitor(SegmentSize{}, fSegments.at(e.first));
            result.push_back(info);
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
            fRegionEventsCV.notify_all();
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
            fRegionEventsCV.notify_all();
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
                auto el = fObservedRegionEvents.find(i.id);
                if (el == fObservedRegionEvents.end()) {
                    fRegionEventCallback(i);
                    fObservedRegionEvents.emplace(i.id, i.event);
                } else {
                    if (el->second == RegionEvent::created && i.event == RegionEvent::destroyed) {
                        fRegionEventCallback(i);
                        el->second = i.event;
                    } else {
                        // LOG(debug) << "ignoring event for id" << i.id << ":";
                        // LOG(debug) << "incoming event: " << i.event;
                        // LOG(debug) << "stored event: " << el->second;
                    }
                }
            }
            fRegionEventsCV.wait(lock);
        }
    }

    void IncrementMsgCounter() { fMsgCounter.fetch_add(1, std::memory_order_relaxed); }
    void DecrementMsgCounter() { fMsgCounter.fetch_sub(1, std::memory_order_relaxed); }

#ifdef FAIRMQ_DEBUG_MODE
    void IncrementShmMsgCounter() { ++(fShmMsgCounter->fCount); }
    void DecrementShmMsgCounter() { --(fShmMsgCounter->fCount); }

    void AddMsgDebug(pid_t pid, size_t size, size_t handle, uint64_t time)
    {
        fMsgDebug->emplace(handle, MsgDebug(pid, size, time));
    }

    void RemoveMsgDebug(size_t handle)
    {
        fMsgDebug->erase(handle);
    }
#endif

    boost::interprocess::named_mutex& GetMtx() { return fShmMtx; }

    void SendHeartbeats()
    {
        std::string controlQueueName("fmq_" + fShmId + "_cq");
        std::unique_lock<std::mutex> lock(fHeartbeatsMtx);
        while (fSendHeartbeats) {
            try {
                boost::interprocess::message_queue mq(boost::interprocess::open_only, controlQueueName.c_str());
                boost::posix_time::ptime sndTill = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(100);
                if (mq.timed_send(fDeviceId.c_str(), fDeviceId.size(), 0, sndTill)) {
                    fHeartbeatsCV.wait_for(lock, std::chrono::milliseconds(100), [&]() { return !fSendHeartbeats; });
                } else {
                    LOG(debug) << "control queue timeout";
                }
            } catch (boost::interprocess::interprocess_exception& ie) {
                fHeartbeatsCV.wait_for(lock, std::chrono::milliseconds(500), [&]() { return !fSendHeartbeats; });
                // LOG(debug) << "no " << controlQueueName << " found";
            }
        }
    }

    bool ThrowingOnBadAlloc() const { return fThrowOnBadAlloc; }

    boost::interprocess::managed_shared_memory::handle_t GetHandleFromAddress(const void* ptr) const
    {
        return boost::apply_visitor(SegmentHandleFromAddress{ptr}, fSegments.at(0));
    }
    void* GetAddressFromHandle(const boost::interprocess::managed_shared_memory::handle_t handle) const
    {
        return boost::apply_visitor(SegmentAddressFromHandle{handle}, fSegments.at(0));
    }

    char* Allocate(const size_t size, size_t alignment = 0)
    {
        char* ptr = nullptr;
        // tools::RateLimiter rateLimiter(20);

        while (ptr == nullptr) {
            try {
                // boost::interprocess::managed_shared_memory::size_type actualSize = size;
                // char* hint = 0; // unused for boost::interprocess::allocate_new
                // ptr = fSegments.at(0).allocation_command<char>(boost::interprocess::allocate_new, size, actualSize, hint);
                size_t segmentSize = boost::apply_visitor(SegmentSize{}, fSegments.at(0));
                if (size > segmentSize) {
                    throw MessageBadAlloc(tools::ToString("Requested message size (", size, ") exceeds segment size (", segmentSize, ")"));
                }
                if (alignment == 0) {
                    ptr = reinterpret_cast<char*>(boost::apply_visitor(SegmentAllocate{size}, fSegments.at(0)));
                } else {
                    ptr = reinterpret_cast<char*>(boost::apply_visitor(SegmentAllocateAligned{size, alignment}, fSegments.at(0)));
                }
            } catch (boost::interprocess::bad_alloc& ba) {
                // LOG(warn) << "Shared memory full...";
                if (ThrowingOnBadAlloc()) {
                    throw MessageBadAlloc(tools::ToString("shmem: could not create a message of size ", size, ", alignment: ", (alignment != 0) ? std::to_string(alignment) : "default", ", free memory: ", boost::apply_visitor(SegmentFreeMemory{}, fSegments.at(0))));
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
            IncrementShmMsgCounter();
            AddMsgDebug(getpid(), size, static_cast<size_t>(GetHandleFromAddress(ptr)), std::chrono::system_clock::now().time_since_epoch().count());
#endif
        }

        return ptr;
    }

    void Deallocate(boost::interprocess::managed_shared_memory::handle_t handle)
    {
        boost::apply_visitor(SegmentDeallocate{GetAddressFromHandle(handle)}, fSegments.at(0));
#ifdef FAIRMQ_DEBUG_MODE
        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(fShmMtx);
        DecrementShmMsgCounter();
        RemoveMsgDebug(handle);
#endif
    }

    char* ShrinkInPlace(size_t oldSize, size_t newSize, char* localPtr)
    {
        return boost::apply_visitor(SegmentBufferShrink{oldSize, newSize, localPtr}, fSegments.at(0));
    }

    ~Manager()
    {
        using namespace boost::interprocess;
        bool lastRemoved = false;

        UnsubscribeFromRegionEvents();

        {
            std::unique_lock<std::mutex> lock(fHeartbeatsMtx);
            fSendHeartbeats = false;
        }
        fHeartbeatsCV.notify_one();
        if (fHeartbeatThread.joinable()) {
            fHeartbeatThread.join();
        }

        try {
            boost::interprocess::scoped_lock<named_mutex> lock(fShmMtx);

            (fDeviceCounter->fCount)--;

            if (fDeviceCounter->fCount == 0) {
                LOG(debug) << "Last segment user, removing segment.";
                lastRemoved = true;
            } else {
                LOG(debug) << "Other segment users present (" << fDeviceCounter->fCount << "), skipping removal.";
            }
        } catch (interprocess_exception& e) {
            LOG(error) << "Manager could not acquire lock: " << e.what();
        }

        if (lastRemoved) {
            Monitor::Cleanup(ShmId{fShmId});
        }
    }

  private:
    std::string fShmId;
    std::string fDeviceId;
    std::unordered_map<uint64_t, boost::variant<RBTreeBestFitSegment, SimpleSeqFitSegment>> fSegments;
    boost::interprocess::managed_shared_memory fManagementSegment;
    VoidAlloc fShmVoidAlloc;
    boost::interprocess::named_mutex fShmMtx;

    boost::interprocess::named_condition fRegionEventsCV;
    std::thread fRegionEventThread;
    bool fRegionEventsSubscriptionActive;
    std::function<void(fair::mq::RegionInfo)> fRegionEventCallback;
    std::unordered_map<uint64_t, RegionEvent> fObservedRegionEvents;

    DeviceCounter* fDeviceCounter;
    Uint64SegmentInfoHashMap* fShmSegments;
    Uint64RegionInfoHashMap* fShmRegions;
    std::unordered_map<uint64_t, std::unique_ptr<Region>> fRegions;

    std::atomic<bool> fInterrupted;
    std::atomic<int32_t> fMsgCounter; // TODO: find a better lifetime solution instead of the counter
#ifdef FAIRMQ_DEBUG_MODE
    SizetMsgDebugMap* fMsgDebug;
    MsgCounter* fShmMsgCounter;
#endif

    std::thread fHeartbeatThread;
    bool fSendHeartbeats;
    std::mutex fHeartbeatsMtx;
    std::condition_variable fHeartbeatsCV;

    bool fThrowOnBadAlloc;
};

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_MANAGER_H_ */

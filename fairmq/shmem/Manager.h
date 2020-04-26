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

#include <FairMQLogger.h>
#include <FairMQUnmanagedRegion.h>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fair
{
namespace mq
{
namespace shmem
{

struct SharedMemoryError : std::runtime_error { using std::runtime_error::runtime_error; };

class Manager
{
    friend struct Region;

  public:
    Manager(const std::string& id, size_t size);

    Manager() = delete;

    Manager(const Manager&) = delete;
    Manager operator=(const Manager&) = delete;

    ~Manager();

    boost::interprocess::managed_shared_memory& Segment() { return fSegment; }
    boost::interprocess::managed_shared_memory& ManagementSegment() { return fManagementSegment; }

    static void StartMonitor(const std::string&);

    void Interrupt() { fInterrupted.store(true); }
    void Resume() { fInterrupted.store(false); }
    bool Interrupted() { return fInterrupted.load(); }

    int GetDeviceCounter();
    int IncrementDeviceCounter();
    int DecrementDeviceCounter();

    std::pair<boost::interprocess::mapped_region*, uint64_t> CreateRegion(const size_t size, const int64_t userFlags, RegionCallback callback, const std::string& path = "", int flags = 0);
    Region* GetRegion(const uint64_t id);
    Region* GetRegionUnsafe(const uint64_t id);
    void RemoveRegion(const uint64_t id);

    std::vector<fair::mq::RegionInfo> GetRegionInfo();
    std::vector<fair::mq::RegionInfo> GetRegionInfoUnsafe();
    void SubscribeToRegionEvents(RegionEventCallback callback);
    void UnsubscribeFromRegionEvents();
    void RegionEventsSubscription();

    void RemoveSegments();

  private:
    std::string fShmId;
    std::string fSegmentName;
    std::string fManagementSegmentName;
    boost::interprocess::managed_shared_memory fSegment;
    boost::interprocess::managed_shared_memory fManagementSegment;
    VoidAlloc fShmVoidAlloc;
    boost::interprocess::named_mutex fShmMtx;

    boost::interprocess::named_condition fRegionEventsCV;
    std::thread fRegionEventThread;
    std::atomic<bool> fRegionEventsSubscriptionActive;
    std::function<void(fair::mq::RegionInfo)> fRegionEventCallback;
    std::unordered_map<uint64_t, RegionEvent> fObservedRegionEvents;

    DeviceCounter* fDeviceCounter;
    Uint64RegionInfoMap* fRegionInfos;
    std::unordered_map<uint64_t, std::unique_ptr<Region>> fRegions;

    std::atomic<bool> fInterrupted;
};

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_MANAGER_H_ */

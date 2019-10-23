/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQShmManager.h
 *
 * @since 2016-04-08
 * @author A. Rybalchenko
 */

#ifndef FAIR_MQ_SHMEM_MANAGER_H_
#define FAIR_MQ_SHMEM_MANAGER_H_

#include <fairmq/Tools.h>
#include <fairmq/shmem/Region.h>
#include <fairmq/shmem/Common.h>

#include "FairMQLogger.h"
#include "FairMQMessage.h"

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include <string>
#include <unordered_map>
#include <stdexcept>

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

    boost::interprocess::managed_shared_memory& Segment();
    boost::interprocess::managed_shared_memory& ManagementSegment();

    void StartMonitor();

    static void Interrupt();
    static void Resume();

    int GetDeviceCounter();
    int IncrementDeviceCounter();
    int DecrementDeviceCounter();

    boost::interprocess::mapped_region* CreateRegion(const size_t size, const uint64_t id, FairMQRegionCallback callback, const std::string& path = "", int flags = 0);
    Region* GetRemoteRegion(const uint64_t id);
    void RemoveRegion(const uint64_t id);

    void RemoveSegments();

  private:
    std::string fShmId;
    std::string fSegmentName;
    std::string fManagementSegmentName;
    boost::interprocess::managed_shared_memory fSegment;
    boost::interprocess::managed_shared_memory fManagementSegment;
    boost::interprocess::named_mutex fShmMtx;
    fair::mq::shmem::DeviceCounter* fDeviceCounter;
    static std::unordered_map<uint64_t, std::unique_ptr<Region>> fRegions;
};

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_MANAGER_H_ */

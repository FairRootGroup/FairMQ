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

#include "FairMQLogger.h"
#include "FairMQMessage.h"
#include "fairmq/Tools.h"
#include "Region.h"
#include "Common.h"

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <thread>
#include <queue>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <condition_variable>

namespace fair
{
namespace mq
{
namespace shmem
{

class Manager
{
    friend class Region;

  public:
    Manager(const std::string& name, size_t size);

    Manager() = delete;

    Manager(const Manager&) = delete;
    Manager operator=(const Manager&) = delete;

    boost::interprocess::managed_shared_memory& Segment();

    void Interrupt();
    void Resume();

    boost::interprocess::mapped_region* CreateRegion(const size_t size, const uint64_t id, FairMQRegionCallback callback);
    boost::interprocess::mapped_region* GetRemoteRegion(const uint64_t id);
    void RemoveRegion(const uint64_t id);

    boost::interprocess::message_queue* GetRegionQueue(const uint64_t id);

    void RemoveSegment();

    boost::interprocess::managed_shared_memory& ManagementSegment();

  private:
    std::string fSessionName;
    std::string fSegmentName;
    std::string fManagementSegmentName;
    boost::interprocess::managed_shared_memory fSegment;
    boost::interprocess::managed_shared_memory fManagementSegment;
    std::unordered_map<uint64_t, Region> fRegions;
};

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_MANAGER_H_ */

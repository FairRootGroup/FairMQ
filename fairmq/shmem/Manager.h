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

#include <string>
#include <unordered_map>

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
    Region* GetRemoteRegion(const uint64_t id);
    void RemoveRegion(const uint64_t id);

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

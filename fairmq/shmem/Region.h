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

#ifndef FAIR_MQ_SHMEM_REGION_H_
#define FAIR_MQ_SHMEM_REGION_H_

#include "FairMQLogger.h"
#include "FairMQUnmanagedRegion.h"

#include <fairmq/Tools.h>
#include <fairmq/shmem/Common.h>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

namespace fair
{
namespace mq
{
namespace shmem
{

class Manager;

struct Region
{
    Region(Manager& manager, uint64_t id, uint64_t size, bool remote, FairMQRegionCallback callback = nullptr, const std::string& path = "", int flags = 0);

    Region() = delete;

    Region(const Region&) = default;
    Region(Region&&) = default;

    void InitializeQueues();

    void StartReceivingAcks();
    void ReceiveAcks();

    void ReleaseBlock(const RegionBlock &);
    void SendAcks();

    ~Region();

    Manager& fManager;
    bool fRemote;
    bool fStop;
    std::string fName;
    std::string fQueueName;
    boost::interprocess::shared_memory_object fShmemObject;
    FILE* fFile;
    boost::interprocess::file_mapping fFileMapping;
    boost::interprocess::mapped_region fRegion;

    std::mutex fBlockLock;
    std::condition_variable fBlockSendCV;
    std::vector<RegionBlock> fBlocksToFree;
    const std::size_t fAckBunchSize = 256;
    std::unique_ptr<boost::interprocess::message_queue> fQueue;

    std::thread fReceiveAcksWorker;
    std::thread fSendAcksWorker;
    FairMQRegionCallback fCallback;
};

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_REGION_H_ */

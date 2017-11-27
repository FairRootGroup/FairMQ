/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Region.h"
#include "Common.h"
#include "Manager.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <chrono>

namespace fair
{
namespace mq
{
namespace shmem
{

using namespace std;

namespace bipc = boost::interprocess;
namespace bpt = boost::posix_time;

Region::Region(Manager& manager, uint64_t id, uint64_t size, bool remote, FairMQRegionCallback callback)
    : fManager(manager)
    , fRemote(remote)
    , fStop(false)
    , fName("fmq_shm_" + fManager.fSessionName +"_region_" + to_string(id))
    , fQueueName("fmq_shm_" + fManager.fSessionName +"_region_queue_" + to_string(id))
    , fShmemObject()
    , fQueue(nullptr)
    , fWorker()
    , fCallback(callback)
{
    if (fRemote)
    {
        fShmemObject = bipc::shared_memory_object(bipc::open_only, fName.c_str(), bipc::read_write);
        LOG(DEBUG) << "shmem: located remote region: " << fName;

        fQueue = fair::mq::tools::make_unique<bipc::message_queue>(bipc::open_only, fQueueName.c_str());
        LOG(DEBUG) << "shmem: located remote region queue: " << fQueueName;
    }
    else
    {
        fShmemObject = bipc::shared_memory_object(bipc::create_only, fName.c_str(), bipc::read_write);
        LOG(DEBUG) << "shmem: created region: " << fName;
        fShmemObject.truncate(size);

        fQueue = fair::mq::tools::make_unique<bipc::message_queue>(bipc::create_only, fQueueName.c_str(), 10000, sizeof(RegionBlock));
        LOG(DEBUG) << "shmem: created region queue: " << fQueueName;
    }
    fRegion = bipc::mapped_region(fShmemObject, bipc::read_write); // TODO: add HUGEPAGES flag here
    // fRegion = bipc::mapped_region(fShmemObject, bipc::read_write, 0, 0, 0, MAP_HUGETLB | MAP_HUGE_1GB);
}

void Region::StartReceivingAcks()
{
    fWorker = std::thread(&Region::ReceiveAcks, this);
}

void Region::ReceiveAcks()
{
    unsigned int priority;
    bipc::message_queue::size_type recvdSize;

    while (!fStop) // end thread condition (should exist until region is destroyed)
    {
        auto rcvTill = bpt::microsec_clock::universal_time() + bpt::milliseconds(200);
        RegionBlock block;
        if (fQueue->timed_receive(&block, sizeof(RegionBlock), recvdSize, priority, rcvTill))
        {
            // LOG(DEBUG) << "received: " << block.fHandle << " " << block.fSize << " " << block.fMessageId;
            if (fCallback)
            {
                fCallback(reinterpret_cast<char*>(fRegion.get_address()) + block.fHandle, block.fSize);
            }
        }
        else
        {
            // LOG(DEBUG) << "queue " << fQueueName << " timeout!";
        }
    } // while !fStop

    LOG(DEBUG) << "worker for " << fName << " leaving.";
}

Region::~Region()
{
    if (!fRemote)
    {
        fStop = true;
        if (fWorker.joinable())
        {
            fWorker.join();
        }

        if (bipc::shared_memory_object::remove(fName.c_str()))
        {
            LOG(DEBUG) << "shmem: destroyed region " << fName;
        }

        if (bipc::message_queue::remove(fQueueName.c_str()))
        {
            LOG(DEBUG) << "shmem: removed region queue " << fName;
        }
    }
    else
    {
        // LOG(DEBUG) << "shmem: region '" << fName << "' is remote, no cleanup necessary.";
        LOG(DEBUG) << "shmem: region queue '" << fQueueName << "' is remote, no cleanup necessary";
    }
}

} // namespace shmem
} // namespace mq
} // namespace fair

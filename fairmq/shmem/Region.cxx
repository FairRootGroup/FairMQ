/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/shmem/Region.h>
#include <fairmq/shmem/Common.h>
#include <fairmq/shmem/Manager.h>

#include <boost/date_time/posix_time/posix_time.hpp>

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
    , fName("fmq_" + fManager.fSessionName +"_rg_" + to_string(id))
    , fQueueName("fmq_" + fManager.fSessionName +"_rgq_" + to_string(id))
    , fShmemObject()
    , fQueue(nullptr)
    , fWorker()
    , fCallback(callback)
{
    if (fRemote)
    {
        fShmemObject = bipc::shared_memory_object(bipc::open_only, fName.c_str(), bipc::read_write);
        LOG(debug) << "shmem: located remote region: " << fName;

        fQueue = fair::mq::tools::make_unique<bipc::message_queue>(bipc::open_only, fQueueName.c_str());
        LOG(debug) << "shmem: located remote region queue: " << fQueueName;
    }
    else
    {
        fShmemObject = bipc::shared_memory_object(bipc::create_only, fName.c_str(), bipc::read_write);
        LOG(debug) << "shmem: created region: " << fName;
        fShmemObject.truncate(size);

        fQueue = fair::mq::tools::make_unique<bipc::message_queue>(bipc::create_only, fQueueName.c_str(), 10000, sizeof(RegionBlock));
        LOG(debug) << "shmem: created region queue: " << fQueueName;
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
            // LOG(debug) << "received: " << block.fHandle << " " << block.fSize << " " << block.fMessageId;
            if (fCallback)
            {
                fCallback(reinterpret_cast<char*>(fRegion.get_address()) + block.fHandle, block.fSize, reinterpret_cast<void*>(block.fHint));
            }
        }
        else
        {
            // LOG(debug) << "queue " << fQueueName << " timeout!";
        }
    } // while !fStop

    LOG(debug) << "worker for " << fName << " leaving.";
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
            LOG(debug) << "shmem: destroyed region " << fName;
        }

        if (bipc::message_queue::remove(fQueueName.c_str()))
        {
            LOG(debug) << "shmem: removed region queue " << fName;
        }
    }
    else
    {
        // LOG(debug) << "shmem: region '" << fName << "' is remote, no cleanup necessary.";
        LOG(debug) << "shmem: region queue '" << fQueueName << "' is remote, no cleanup necessary";
    }
}

} // namespace shmem
} // namespace mq
} // namespace fair

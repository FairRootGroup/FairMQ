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

#include <chrono>

using namespace std;

namespace bipc = ::boost::interprocess;
namespace bpt = ::boost::posix_time;

namespace fair
{
namespace mq
{
namespace shmem
{

Region::Region(Manager& manager, uint64_t id, uint64_t size, bool remote, FairMQRegionCallback callback)
    : fManager(manager)
    , fRemote(remote)
    , fStop(false)
    , fName("fmq_" + fManager.fSessionName +"_rg_" + to_string(id))
    , fQueueName("fmq_" + fManager.fSessionName +"_rgq_" + to_string(id))
    , fShmemObject()
    , fQueue(nullptr)
    , fReceiveAcksWorker()
    , fSendAcksWorker()
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

        fQueue = fair::mq::tools::make_unique<bipc::message_queue>(bipc::create_only, fQueueName.c_str(), 1024, fAckBunchSize * sizeof(RegionBlock));
        LOG(debug) << "shmem: created region queue: " << fQueueName;
    }
    fRegion = bipc::mapped_region(fShmemObject, bipc::read_write); // TODO: add HUGEPAGES flag here
    // fRegion = bipc::mapped_region(fShmemObject, bipc::read_write, 0, 0, 0, MAP_ANONYMOUS | MAP_HUGETLB);

    fSendAcksWorker = std::thread(&Region::SendAcks, this);
}

void Region::StartReceivingAcks()
{
    fReceiveAcksWorker = std::thread(&Region::ReceiveAcks, this);
}

void Region::ReceiveAcks()
{
    unsigned int priority;
    bipc::message_queue::size_type recvdSize;
    std::unique_ptr<RegionBlock[]> blocks = fair::mq::tools::make_unique<RegionBlock[]>(fAckBunchSize);

    while (!fStop) // end thread condition (should exist until region is destroyed)
    {
        auto rcvTill = bpt::microsec_clock::universal_time() + bpt::milliseconds(500);

        while (fQueue->timed_receive(blocks.get(), fAckBunchSize * sizeof(RegionBlock), recvdSize, priority, rcvTill))
        {
            // LOG(debug) << "received: " << block.fHandle << " " << block.fSize << " " << block.fMessageId;
            if (fCallback)
            {
                const auto numBlocks = recvdSize / sizeof(RegionBlock);
                for (size_t i = 0; i < numBlocks; i++)
                {
                    fCallback(reinterpret_cast<char*>(fRegion.get_address()) + blocks[i].fHandle, blocks[i].fSize, reinterpret_cast<void*>(blocks[i].fHint));
                }
            }
        }
    } // while !fStop

    LOG(debug) << "receive ack worker for " << fName << " leaving.";
}

void Region::ReleaseBlock(const RegionBlock &block)
{
    std::unique_lock<std::mutex> lock(fBlockLock);

    fBlocksToFree.emplace_back(block);

    if (fBlocksToFree.size() >= fAckBunchSize)
    {
        lock.unlock(); // reduces contention on fBlockLock
        fBlockSendCV.notify_one();
    }
}

void Region::SendAcks()
{
    std::unique_ptr<RegionBlock[]> blocks = fair::mq::tools::make_unique<RegionBlock[]>(fAckBunchSize);

    while (true) // we'll try to send all acks before stopping
    {
        size_t blocksToSend = 0;

        {   // mutex locking block
            std::unique_lock<std::mutex> lock(fBlockLock);

            // try to get more blocks without waiting (we can miss a notify from CloseMessage())
            if (!fStop && (fBlocksToFree.size() < fAckBunchSize))
            {
                // cv.wait() timeout: send whatever blocks we have
                fBlockSendCV.wait_for(lock, std::chrono::milliseconds(500));
            }

            blocksToSend = std::min(fBlocksToFree.size(), fAckBunchSize);

            std::copy_n(fBlocksToFree.end() - blocksToSend, blocksToSend, blocks.get());
            fBlocksToFree.resize(fBlocksToFree.size() - blocksToSend);
        } // unlock the block mutex here while sending over IPC

        if (blocksToSend > 0)
        {
            while (!fQueue->try_send(blocks.get(), blocksToSend * sizeof(RegionBlock), 0) && !fStop)
            {
                // receiver slow? yield and try again...
                this_thread::yield();
            }
        }
        else // blocksToSend == 0
        {
            if (fStop)
            {
                break;
            }
        }
    }

    LOG(debug) << "send ack worker for " << fName << " leaving.";
}

Region::~Region()
{
    fStop = true;

    if (fSendAcksWorker.joinable())
    {
        fSendAcksWorker.join();
    }

    if (!fRemote)
    {
        if (fReceiveAcksWorker.joinable())
        {
            fReceiveAcksWorker.join();
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

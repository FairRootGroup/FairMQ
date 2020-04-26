/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Region.h"
#include "Common.h"
#include "Manager.h"

#include <fairmq/tools/CppSTL.h>
#include <fairmq/tools/Strings.h>

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <cerrno>
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

Region::Region(Manager& manager, uint64_t id, uint64_t size, bool remote, RegionCallback callback, const string& path /* = "" */, int flags /* = 0 */)
    : fManager(manager)
    , fRemote(remote)
    , fStop(false)
    , fName("fmq_" + fManager.fShmId + "_rg_" + to_string(id))
    , fQueueName("fmq_" + fManager.fShmId + "_rgq_" + to_string(id))
    , fShmemObject()
    , fFile(nullptr)
    , fFileMapping()
    , fQueue(nullptr)
    , fReceiveAcksWorker()
    , fSendAcksWorker()
    , fCallback(callback)
{
    if (path != "") {
        fName = string(path + fName);

        fFile = fopen(fName.c_str(), fRemote ? "r+" : "w+");

        if (!fFile) {
            LOG(error) << "Failed to initialize file: " << fName;
            LOG(error) << "errno: " << errno << ": " << strerror(errno);
            throw runtime_error(tools::ToString("Failed to initialize file for shared memory region: ", strerror(errno)));
        }
        fFileMapping = bipc::file_mapping(fName.c_str(), bipc::read_write);
        LOG(debug) << "shmem: initialized file: " << fName;
        fRegion = bipc::mapped_region(fFileMapping, bipc::read_write, 0, size, 0, flags);
    } else {
        if (fRemote) {
            fShmemObject = bipc::shared_memory_object(bipc::open_only, fName.c_str(), bipc::read_write);
        } else {
            fShmemObject = bipc::shared_memory_object(bipc::create_only, fName.c_str(), bipc::read_write);
            fShmemObject.truncate(size);
        }
        fRegion = bipc::mapped_region(fShmemObject, bipc::read_write, 0, 0, 0, flags);
    }

    InitializeQueues();
    LOG(debug) << "shmem: initialized region: " << fName;
}

void Region::InitializeQueues()
{
    if (fRemote) {
        fQueue = tools::make_unique<bipc::message_queue>(bipc::open_only, fQueueName.c_str());
    } else {
        fQueue = tools::make_unique<bipc::message_queue>(bipc::create_only, fQueueName.c_str(), 1024, fAckBunchSize * sizeof(RegionBlock));
    }
    LOG(debug) << "shmem: initialized region queue: " << fQueueName;
}

void Region::StartSendingAcks()
{
    fSendAcksWorker = thread(&Region::SendAcks, this);
}

void Region::StartReceivingAcks()
{
    fReceiveAcksWorker = thread(&Region::ReceiveAcks, this);
}

void Region::ReceiveAcks()
{
    unsigned int priority;
    bipc::message_queue::size_type recvdSize;
    unique_ptr<RegionBlock[]> blocks = tools::make_unique<RegionBlock[]>(fAckBunchSize);

    while (!fStop) { // end thread condition (should exist until region is destroyed)
        auto rcvTill = bpt::microsec_clock::universal_time() + bpt::milliseconds(500);

        while (fQueue->timed_receive(blocks.get(), fAckBunchSize * sizeof(RegionBlock), recvdSize, priority, rcvTill)) {
            // LOG(debug) << "received: " << block.fHandle << " " << block.fSize << " " << block.fMessageId;
            if (fCallback) {
                const auto numBlocks = recvdSize / sizeof(RegionBlock);
                for (size_t i = 0; i < numBlocks; i++) {
                    fCallback(reinterpret_cast<char*>(fRegion.get_address()) + blocks[i].fHandle, blocks[i].fSize, reinterpret_cast<void*>(blocks[i].fHint));
                }
            }
        }
    } // while !fStop

    LOG(debug) << "receive ack worker for " << fName << " leaving.";
}

void Region::ReleaseBlock(const RegionBlock &block)
{
    unique_lock<mutex> lock(fBlockMtx);

    fBlocksToFree.emplace_back(block);

    if (fBlocksToFree.size() >= fAckBunchSize) {
        lock.unlock(); // reduces contention on fBlockMtx
        fBlockSendCV.notify_one();
    }
}

void Region::SendAcks()
{
    unique_ptr<RegionBlock[]> blocks = tools::make_unique<RegionBlock[]>(fAckBunchSize);

    while (true) { // we'll try to send all acks before stopping
        size_t blocksToSend = 0;

        {   // mutex locking block
            unique_lock<mutex> lock(fBlockMtx);

            // try to get more blocks without waiting (we can miss a notify from CloseMessage())
            if (!fStop && (fBlocksToFree.size() < fAckBunchSize)) {
                // cv.wait() timeout: send whatever blocks we have
                fBlockSendCV.wait_for(lock, chrono::milliseconds(500));
            }

            blocksToSend = min(fBlocksToFree.size(), fAckBunchSize);

            copy_n(fBlocksToFree.end() - blocksToSend, blocksToSend, blocks.get());
            fBlocksToFree.resize(fBlocksToFree.size() - blocksToSend);
        } // unlock the block mutex here while sending over IPC

        if (blocksToSend > 0) {
            while (!fQueue->try_send(blocks.get(), blocksToSend * sizeof(RegionBlock), 0) && !fStop) {
                // receiver slow? yield and try again...
                this_thread::yield();
            }
        } else { // blocksToSend == 0
            if (fStop) {
                break;
            }
        }
    }

    LOG(debug) << "send ack worker for " << fName << " leaving.";
}

Region::~Region()
{
    fStop = true;

    if (fSendAcksWorker.joinable()) {
        fBlockSendCV.notify_one();
        fSendAcksWorker.join();
    }

    if (!fRemote) {
        if (fReceiveAcksWorker.joinable()) {
            fReceiveAcksWorker.join();
        }

        if (bipc::shared_memory_object::remove(fName.c_str())) {
            LOG(debug) << "shmem: destroyed region " << fName;
        }

        if (bipc::file_mapping::remove(fName.c_str())) {
            LOG(debug) << "shmem: destroyed file mapping " << fName;
        }

        if (fFile) {
            fclose(fFile);
        }

        if (bipc::message_queue::remove(fQueueName.c_str())) {
            LOG(debug) << "shmem: removed region queue " << fQueueName;
        }
    } else {
        // LOG(debug) << "shmem: region '" << fName << "' is remote, no cleanup necessary.";
        LOG(debug) << "shmem: region queue '" << fQueueName << "' is remote, no cleanup necessary";
    }
}

} // namespace shmem
} // namespace mq
} // namespace fair

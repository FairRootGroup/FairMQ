/********************************************************************************
*    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
*                                                                              *
*              This software is distributed under the terms of the             *
*              GNU Lesser General Public Licence (LGPL) version 3,             *
*                  copied verbatim in the file "LICENSE"                       *
********************************************************************************/
/**
* Region.h
*
* @since 2016-04-08
* @author A. Rybalchenko
*/

#ifndef FAIR_MQ_SHMEM_REGION_H_
#define FAIR_MQ_SHMEM_REGION_H_

#include "Common.h"

#include <FairMQLogger.h>
#include <FairMQUnmanagedRegion.h>
#include <fairmq/tools/CppSTL.h>
#include <fairmq/tools/Strings.h>

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <algorithm> // min
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <cerrno>
#include <chrono>
#include <ios>

namespace fair
{
namespace mq
{
namespace shmem
{

struct Region
{
    Region(const std::string& shmId, uint64_t id, uint64_t size, bool remote, RegionCallback callback, RegionBulkCallback bulkCallback, const std::string& path, int flags)
        : fRemote(remote)
        , fLinger(100)
        , fStop(false)
        , fName("fmq_" + shmId + "_rg_" + std::to_string(id))
        , fQueueName("fmq_" + shmId + "_rgq_" + std::to_string(id))
        , fShmemObject()
        , fFile(nullptr)
        , fFileMapping()
        , fQueue(nullptr)
        , fReceiveAcksWorker()
        , fSendAcksWorker()
        , fCallback(callback)
        , fBulkCallback(bulkCallback)
    {
        using namespace boost::interprocess;

        if (path != "") {
            fName = std::string(path + fName);

            if (!fRemote) {
                // create a file
                std::filebuf fbuf;
                if (fbuf.open(fName, std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios_base::binary)) {
                    // set the size
                    fbuf.pubseekoff(size - 1, std::ios_base::beg);
                    fbuf.sputc(0);
                }
            }

            fFile = fopen(fName.c_str(), "r+");

            if (!fFile) {
                LOG(error) << "Failed to initialize file: " << fName;
                LOG(error) << "errno: " << errno << ": " << strerror(errno);
                throw std::runtime_error(tools::ToString("Failed to initialize file for shared memory region: ", strerror(errno)));
            }
            fFileMapping = file_mapping(fName.c_str(), read_write);
            LOG(debug) << "shmem: initialized file: " << fName;
            fRegion = mapped_region(fFileMapping, read_write, 0, size, 0, flags);
        } else {
            if (fRemote) {
                fShmemObject = shared_memory_object(open_only, fName.c_str(), read_write);
            } else {
                fShmemObject = shared_memory_object(create_only, fName.c_str(), read_write);
                fShmemObject.truncate(size);
            }
            fRegion = mapped_region(fShmemObject, read_write, 0, 0, 0, flags);
        }

        InitializeQueues();
        StartSendingAcks();
        LOG(debug) << "shmem: initialized region: " << fName;
    }

    Region() = delete;

    Region(const Region&) = delete;
    Region(Region&&) = delete;

    void InitializeQueues()
    {
        using namespace boost::interprocess;

        if (fRemote) {
            fQueue = tools::make_unique<message_queue>(open_only, fQueueName.c_str());
        } else {
            fQueue = tools::make_unique<message_queue>(create_only, fQueueName.c_str(), 1024, fAckBunchSize * sizeof(RegionBlock));
        }
        LOG(debug) << "shmem: initialized region queue: " << fQueueName;
    }

    void StartSendingAcks()
    {
        fSendAcksWorker = std::thread(&Region::SendAcks, this);
    }

    void SendAcks()
    {
        std::unique_ptr<RegionBlock[]> blocks = tools::make_unique<RegionBlock[]>(fAckBunchSize);

        while (true) { // we'll try to send all acks before stopping
            size_t blocksToSend = 0;

            {   // mutex locking block
                std::unique_lock<std::mutex> lock(fBlockMtx);

                // try to get more blocks without waiting (we can miss a notify from CloseMessage())
                if (!fStop && (fBlocksToFree.size() < fAckBunchSize)) {
                    // cv.wait() timeout: send whatever blocks we have
                    fBlockSendCV.wait_for(lock, std::chrono::milliseconds(500));
                }

                blocksToSend = std::min(fBlocksToFree.size(), fAckBunchSize);

                copy_n(fBlocksToFree.end() - blocksToSend, blocksToSend, blocks.get());
                fBlocksToFree.resize(fBlocksToFree.size() - blocksToSend);
            } // unlock the block mutex here while sending over IPC

            if (blocksToSend > 0) {
                while (!fQueue->try_send(blocks.get(), blocksToSend * sizeof(RegionBlock), 0) && !fStop) {
                    // receiver slow? yield and try again...
                    std::this_thread::yield();
                }
            } else { // blocksToSend == 0
                if (fStop) {
                    break;
                }
            }
        }

        LOG(debug) << "send ack worker for " << fName << " leaving.";
    }

    void StartReceivingAcks()
    {
        fReceiveAcksWorker = std::thread(&Region::ReceiveAcks, this);
    }

    void ReceiveAcks()
    {
        unsigned int priority;
        boost::interprocess::message_queue::size_type recvdSize;
        std::unique_ptr<RegionBlock[]> blocks = tools::make_unique<RegionBlock[]>(fAckBunchSize);
        std::vector<fair::mq::RegionBlock> result;
        result.reserve(fAckBunchSize);

        while (!fStop) { // end thread condition (should exist until region is destroyed)
            auto rcvTill = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(500);

            while (fQueue->timed_receive(blocks.get(), fAckBunchSize * sizeof(RegionBlock), recvdSize, priority, rcvTill)) {
                // LOG(debug) << "received: " << block.fHandle << " " << block.fSize << " " << block.fMessageId;
                const auto numBlocks = recvdSize / sizeof(RegionBlock);
                if (fBulkCallback) {
                    result.clear();
                    for (size_t i = 0; i < numBlocks; i++) {
                        result.emplace_back(reinterpret_cast<char*>(fRegion.get_address()) + blocks[i].fHandle, blocks[i].fSize, reinterpret_cast<void*>(blocks[i].fHint));
                    }
                    fBulkCallback(result);
                } else if (fCallback) {
                    for (size_t i = 0; i < numBlocks; i++) {
                        fCallback(reinterpret_cast<char*>(fRegion.get_address()) + blocks[i].fHandle, blocks[i].fSize, reinterpret_cast<void*>(blocks[i].fHint));
                    }
                }
            }
        } // while !fStop

        LOG(debug) << "ReceiveAcks() worker for " << fName << " leaving.";
    }

    void ReleaseBlock(const RegionBlock& block)
    {
        std::unique_lock<std::mutex> lock(fBlockMtx);

        fBlocksToFree.emplace_back(block);

        if (fBlocksToFree.size() >= fAckBunchSize) {
            lock.unlock(); // reduces contention on fBlockMtx
            fBlockSendCV.notify_one();
        }
    }

    void SetLinger(uint32_t linger) { fLinger = linger; }
    uint32_t GetLinger() const { return fLinger; }

    ~Region()
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

            if (boost::interprocess::shared_memory_object::remove(fName.c_str())) {
                LOG(debug) << "shmem: destroyed region " << fName;
            }

            if (boost::interprocess::file_mapping::remove(fName.c_str())) {
                LOG(debug) << "shmem: destroyed file mapping " << fName;
            }

            if (fFile) {
                fclose(fFile);
            }

            if (boost::interprocess::message_queue::remove(fQueueName.c_str())) {
                LOG(debug) << "shmem: removed region queue " << fQueueName;
            }
        } else {
            // LOG(debug) << "shmem: region '" << fName << "' is remote, no cleanup necessary.";
            LOG(debug) << "shmem: region queue '" << fQueueName << "' is remote, no cleanup necessary";
        }
    }

    bool fRemote;
    uint32_t fLinger;
    std::atomic<bool> fStop;
    std::string fName;
    std::string fQueueName;
    boost::interprocess::shared_memory_object fShmemObject;
    FILE* fFile;
    boost::interprocess::file_mapping fFileMapping;
    boost::interprocess::mapped_region fRegion;

    std::mutex fBlockMtx;
    std::condition_variable fBlockSendCV;
    std::vector<RegionBlock> fBlocksToFree;
    const std::size_t fAckBunchSize = 256;
    std::unique_ptr<boost::interprocess::message_queue> fQueue;

    std::thread fReceiveAcksWorker;
    std::thread fSendAcksWorker;
    RegionCallback fCallback;
    RegionBulkCallback fBulkCallback;
};

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_REGION_H_ */

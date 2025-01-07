/********************************************************************************
 * Copyright (C) 2014-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SHMEM_UNMANAGEDREGION_H_
#define FAIR_MQ_SHMEM_UNMANAGEDREGION_H_

#include <fairmq/shmem/Common.h>
#include <fairmq/shmem/Monitor.h>
#include <fairmq/tools/Strings.h>
#include <fairmq/UnmanagedRegion.h>
#include <fairmq/Transports.h>

#include <fairlogger/Logger.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <algorithm> // min
#include <atomic>
#include <thread>
#include <memory> // make_unique
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <cerrno>
#include <chrono>
#include <ios>
#include <utility> // move

namespace fair::mq::shmem
{

struct UnmanagedRegion
{
    friend class Message;
    friend class Manager;
    friend class Monitor;

    UnmanagedRegion(const std::string& shmId, uint16_t id, uint64_t size)
        : UnmanagedRegion(shmId,  size, true, makeRegionConfig(id))
    {}

    UnmanagedRegion(const std::string& shmId, uint64_t size, RegionConfig cfg)
        : UnmanagedRegion(shmId, size, true, std::move(cfg))
    {}

    UnmanagedRegion(const std::string& shmId, RegionConfig cfg)
        : UnmanagedRegion(shmId, cfg.size, true, std::move(cfg))
    {}

    UnmanagedRegion(const std::string& shmId, uint64_t size, bool controlling, RegionConfig cfg)
        : fControlling(controlling)
        , fRemoveOnDestruction(cfg.removeOnDestruction)
        , fLinger(cfg.linger)
        , fStopAcks(false)
        , fName(MakeShmName(shmId, "rg", cfg.id.value()))
        , fQueueName(MakeShmName(shmId, "rgq", cfg.id.value()))
        , fRefCountSegmentName(MakeShmName(shmId, "rrc", cfg.id.value()))
        , fShmemObject()
        , fFile(nullptr)
        , fFileMapping()
        , fRcSegmentSize(cfg.rcSegmentSize)
        , fQueue(nullptr)
        , fCallback(nullptr)
        , fBulkCallback(nullptr)
    {
        using namespace boost::interprocess;

        // TODO: refactor this
        cfg.size = size;
        const uint16_t id = cfg.id.value();
        bool created = false;

        LOG(debug) << "UnmanagedRegion(): " << fName << " (" << (fControlling ? "controller" : "viewer") << ")";

        if (!cfg.path.empty()) {
            fName = std::string(cfg.path + fName);

            if (fControlling) {
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
                throw TransportError(tools::ToString("Failed to initialize file for shared memory region: ", strerror(errno)));
            }
            fFileMapping = file_mapping(fName.c_str(), read_write);
            LOG(debug) << "UnmanagedRegion(): initialized file: " << fName;
            fRegion = mapped_region(fFileMapping, read_write, 0, size, 0, cfg.creationFlags);
        } else {
            try {
                // if opening fails, create
                try {
                    fShmemObject = shared_memory_object(open_only, fName.c_str(), read_write);
                    created = false;
                } catch (interprocess_exception& e) {
                    if (fControlling) {
                        LOG(debug) << "Could not open controlling shared_memory_object for region " << id << ": " << e.what() << ", creating...";
                        fShmemObject = shared_memory_object(create_only, fName.c_str(), read_write);
                        fShmemObject.truncate(size);
                        created = true;
                    } else {
                        LOG(error) << "Could not open view for shared_memory_object for region " << id << ": " << e.what();
                        throw TransportError(tools::ToString("Could not open view for shared_memory_object for region ", id, ": ", e.what()));
                    }
                }
            } catch (interprocess_exception& e) {
                LOG(error) << "Failed initializing shared_memory_object for region id " << id << ": " << e.what();
                throw;
            }

            try {
                fRegion = mapped_region(fShmemObject, read_write, 0, 0, 0, cfg.creationFlags);
                if (size != 0 && size != fRegion.get_size()) {
                    LOG(error) << "Created/opened region size (" << fRegion.get_size() << ") does not match configured size (" << size << ")";
                    throw TransportError(tools::ToString("Created/opened region size (", fRegion.get_size(), ") does not match configured size (", size, ")"));
                }
            } catch (interprocess_exception& e) {
                LOG(error) << "Failed mapping shared_memory_object for region id " << id << ": " << e.what();
                throw;
            }
        }

        if (cfg.lock) {
            LOG(debug) << "Locking region " << id << "...";
            Lock();
            LOG(debug) << "Successfully locked region " << id << ".";
        }
        if (cfg.zero) {
            LOG(debug) << "Zeroing free memory of region " << id << "...";
            Zero();
            LOG(debug) << "Successfully zeroed free memory of region " << id << ".";
        }

        InitializeRefCountSegment(fRcSegmentSize);

        if (fControlling && created) {
            Register(shmId, cfg);
        }

        LOG(debug) << (created ? "Created" : "Opened") << " unmanaged shared memory region: " << fName << " (" << (fControlling ? "controller" : "viewer") << "), refCount segment size: " << fRcSegmentSize;
    }

    UnmanagedRegion() = delete;

    UnmanagedRegion(const UnmanagedRegion&) = delete;
    UnmanagedRegion(UnmanagedRegion&&) = delete;
    UnmanagedRegion& operator=(const UnmanagedRegion&) = delete;
    UnmanagedRegion& operator=(UnmanagedRegion&&) = delete;

    void BecomeController(RegionConfig& cfg)
    {
        fControlling = true;
        fLinger = cfg.linger;
        fRemoveOnDestruction = cfg.removeOnDestruction;
    }

    void Zero()
    {
        memset(fRegion.get_address(), 0x00, fRegion.get_size());
    }
    void Lock()
    {
        if (mlock(fRegion.get_address(), fRegion.get_size()) == -1) {
            LOG(error) << "Could not lock region " << fName << ". Code: " << errno << ", reason: " << strerror(errno);
            throw TransportError(tools::ToString("Could not lock region ", fName, ": ", strerror(errno)));
        }
    }

    void* GetData() const { return fRegion.get_address(); }
    size_t GetSize() const { return fRegion.get_size(); }

    void SetLinger(uint32_t linger) { fLinger = linger; }
    uint32_t GetLinger() const { return fLinger; }

    bool RemoveOnDestruction() { return fRemoveOnDestruction; }

    RefCount& MakeRefCount(uint16_t initialCount = 1)
    {
        RefCount* refCount = fRefCountPool->allocate_one().get();
        new (refCount) RefCount(initialCount);
        return *refCount;
    }

    void RemoveRefCount(RefCount& refCount)
    {
        refCount.~RefCount();
        fRefCountPool->deallocate_one(&refCount);
    }

    ~UnmanagedRegion()
    {
        LOG(debug) << "~UnmanagedRegion(): " << fName << " (" << (fControlling ? "controller" : "viewer") << ")";
        fStopAcks = true;

        if (fAcksSender.joinable()) {
            fBlockSendCV.notify_one();
            fAcksSender.join();
        }

        if (fControlling) {
            if (fAcksReceiver.joinable()) {
                fAcksReceiver.join();
            }

            if (fRemoveOnDestruction) {
                if (Monitor::RemoveObject(fName.c_str())) {
                    LOG(trace) << "Region '" << fName << "' destroyed.";
                }
                if (Monitor::RemoveFileMapping(fName.c_str())) {
                    LOG(trace) << "File mapping '" << fName << "' destroyed.";
                }
                if (fRefCountSegment) {
                    if (Monitor::RemoveObject(fRefCountSegmentName)) {
                        LOG(trace) << "Ref Count Segment '" << fRefCountSegmentName << "' destroyed.";
                    }
                }
            } else {
                LOG(debug) << "Skipping removal of " << fName << " unmanaged region, because RegionConfig::removeOnDestruction is false";
            }

            if (boost::interprocess::message_queue::remove(fQueueName.c_str())) {
                LOG(trace) << "Region queue '" << fQueueName << "' destroyed.";
            } else {
                LOG(debug) << "Region queue '" << fQueueName << "' not destroyed.";
            }

            if (fFile) {
                fclose(fFile);
            }
        } else {
            // LOG(debug) << "Region queue '" << fQueueName << "' is viewer, no cleanup necessary";
        }

        // LOG(debug) << "Region '" << fName << "' (" << (fControlling ? "controller" : "viewer") << ") destructed.";
    }

  private:
    bool fControlling;
    bool fRemoveOnDestruction;
    uint32_t fLinger;
    std::atomic<bool> fStopAcks;
    std::string fName;
    std::string fQueueName;
    std::string fRefCountSegmentName;
    boost::interprocess::shared_memory_object fShmemObject;
    FILE* fFile;
    boost::interprocess::file_mapping fFileMapping;
    boost::interprocess::mapped_region fRegion;

    std::mutex fBlockMtx;
    std::condition_variable fBlockSendCV;
    std::vector<RegionBlock> fBlocksToFree;
    const std::size_t fAckBunchSize = 256;
    uint64_t fRcSegmentSize;
    std::unique_ptr<boost::interprocess::message_queue> fQueue;
    std::unique_ptr<boost::interprocess::managed_shared_memory> fRefCountSegment;
    std::unique_ptr<RefCountPool> fRefCountPool;

    std::thread fAcksReceiver;
    std::thread fAcksSender;
    RegionCallback fCallback;
    RegionBulkCallback fBulkCallback;

    static RegionConfig makeRegionConfig(uint16_t id)
    {
        RegionConfig regionCfg;
        regionCfg.id = id;
        return regionCfg;
    }

    static void Register(const std::string& shmId, const RegionConfig& cfg)
    {
        using namespace boost::interprocess;
        LOG(debug) << "Registering unmanaged shared memory region with id " << cfg.id.value();
        managed_shared_memory mngSegment(open_or_create, MakeShmName(shmId, "mng").c_str(), kManagementSegmentSize);
        VoidAlloc alloc(mngSegment.get_segment_manager());

        Uint16RegionInfoHashMap* shmRegions = mngSegment.find_or_construct<Uint16RegionInfoHashMap>(unique_instance)(alloc);

        EventCounter* eventCounter = mngSegment.find_or_construct<EventCounter>(unique_instance)(0);

        auto it = shmRegions->find(cfg.id.value());
        if (it != shmRegions->end()) {
            LOG(error) << "Unmanaged Region with id " << cfg.id.value() << " has already been registered. Only unique IDs per session are allowed.";
            throw TransportError(tools::ToString("Unmanaged Region with id ", cfg.id.value(), " has already been registered. Only unique IDs per session are allowed."));
        }

        shmRegions->emplace(cfg.id.value(), RegionInfo(cfg.path.c_str(), cfg.creationFlags, cfg.userFlags, cfg.size, cfg.rcSegmentSize, alloc));
        (eventCounter->fCount)++;
    }

    void SetCallbacks(RegionCallback callback, RegionBulkCallback bulkCallback)
    {
        fCallback = std::move(callback);
        fBulkCallback = std::move(bulkCallback);
    }

    void InitializeQueues()
    {
        using namespace boost::interprocess;
        if (!fQueue) {
            fQueue = std::make_unique<message_queue>(open_or_create, fQueueName.c_str(), 1024, fAckBunchSize * sizeof(RegionBlock));
            LOG(trace) << "shmem: initialized region queue: " << fQueueName;
        }
    }

    void InitializeRefCountSegment(uint64_t size)
    {
        using namespace boost::interprocess;
        if (!fRefCountSegment && size > 0) {
            fRefCountSegment = std::make_unique<managed_shared_memory>(open_or_create, fRefCountSegmentName.c_str(), size);
            LOG(trace) << "shmem: initialized ref count segment: " << fRefCountSegmentName;
            fRefCountPool = std::make_unique<RefCountPool>(fRefCountSegment->get_segment_manager());
        }
    }

    RefCount* GetRefCountAddressFromHandle(const boost::interprocess::managed_shared_memory::handle_t handle)
    {
        if (fRefCountPool) {
            return reinterpret_cast<RefCount*>(fRefCountSegment->get_address_from_handle(handle));
        }
        return nullptr;
    };

    boost::interprocess::managed_shared_memory::handle_t HandleFromAddress(const void* ptr)
    {
        return fRefCountSegment->get_handle_from_address(ptr);
    }

    void StartAckSender()
    {
        if (!fAcksSender.joinable()) {
            fAcksSender = std::thread(&UnmanagedRegion::SendAcks, this);
        }
    }
    void SendAcks()
    {
        std::unique_ptr<RegionBlock[]> blocks = std::make_unique<RegionBlock[]>(fAckBunchSize);
        size_t blocksToSend = 0;

        while (true) {
            {
                std::unique_lock<std::mutex> lock(fBlockMtx);

                // try to get <fAckBunchSize> blocks
                if (fBlocksToFree.size() < fAckBunchSize) {
                    fBlockSendCV.wait_for(lock, std::chrono::milliseconds(500));
                }

                // send whatever blocks we have
                blocksToSend = std::min(fBlocksToFree.size(), fAckBunchSize);

                copy_n(fBlocksToFree.end() - blocksToSend, blocksToSend, blocks.get());
                fBlocksToFree.resize(fBlocksToFree.size() - blocksToSend);
            }

            if (blocksToSend > 0) {
                while (!fQueue->try_send(blocks.get(), blocksToSend * sizeof(RegionBlock), 0) && !fStopAcks) {
                    // receiver slow? yield and try again...
                    std::this_thread::yield();
                }
                // LOG(debug) << "Sent " << blocksToSend << " blocks.";
            } else { // blocksToSend == 0
                if (fStopAcks) {
                    break;
                }
            }
        }

        LOG(trace) << "AcksSender for " << fName << " leaving " << "(blocks left to free: " << fBlocksToFree.size() << ", "
                                                                << " blocks left to send: " << blocksToSend << ").";
    }

    void StartAckReceiver()
    {
        if (!fAcksReceiver.joinable()) {
            fAcksReceiver = std::thread(&UnmanagedRegion::ReceiveAcks, this);
        }
    }
    void ReceiveAcks()
    {
        unsigned int priority = 0;
        boost::interprocess::message_queue::size_type recvdSize = 0;
        std::unique_ptr<RegionBlock[]> blocks = std::make_unique<RegionBlock[]>(fAckBunchSize);
        std::vector<fair::mq::RegionBlock> result;
        result.reserve(fAckBunchSize);

        while (true) {
            uint32_t timeout = 100;
            bool leave = false;
            if (fStopAcks) {
                timeout = fLinger;
                leave = true;
            }
            auto rcvTill = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::milliseconds(timeout);

            while (fQueue->timed_receive(blocks.get(), fAckBunchSize * sizeof(RegionBlock), recvdSize, priority, rcvTill)) {
                const auto numBlocks = recvdSize / sizeof(RegionBlock);
                // LOG(debug) << "Received " << numBlocks << " blocks (recvdSize: " << recvdSize << "). (remaining queue size: " << fQueue->get_num_msg() << ").";
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

            if (leave) {
                break;
            }
        }

        LOG(trace) << "AcksReceiver for " << fName << " leaving (remaining queue size: " << fQueue->get_num_msg() << ").";
    }

    void ReleaseBlock(const RegionBlock& block)
    {
        std::unique_lock<std::mutex> lock(fBlockMtx);

        fBlocksToFree.emplace_back(block);

        if (fBlocksToFree.size() >= fAckBunchSize) {
            lock.unlock();
            fBlockSendCV.notify_one();
        }
    }

    void StopAcks()
    {
        fStopAcks = true;

        if (fAcksSender.joinable()) {
            fBlockSendCV.notify_one();
            fAcksSender.join();
        }

        if (fAcksReceiver.joinable()) {
            fAcksReceiver.join();
        }
    }
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_UNMANAGEDREGION_H_ */

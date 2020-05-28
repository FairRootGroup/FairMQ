/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_SHMEM_MESSAGE_H_
#define FAIR_MQ_SHMEM_MESSAGE_H_

#include "Common.h"
#include "Manager.h"
#include "Region.h"
#include "UnmanagedRegion.h"
#include <fairmq/Tools.h>
#include <FairMQLogger.h>
#include <FairMQMessage.h>
#include <FairMQUnmanagedRegion.h>

#include <boost/interprocess/mapped_region.hpp>

#include <cstddef> // size_t
#include <atomic>

namespace fair
{
namespace mq
{
namespace shmem
{

class Socket;

class Message final : public fair::mq::Message
{
    friend class Socket;

  public:
    Message(Manager& manager, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{0, 0, 0, -1}
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
    {
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, Alignment /* alignment */, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{0, 0, 0, -1}
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
    {
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, const size_t size, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{0, 0, 0, -1}
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
    {
        InitializeChunk(size);
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, const size_t size, Alignment alignment, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{0, 0, 0, -1}
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
    {
        InitializeChunk(size, static_cast<size_t>(alignment));
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{0, 0, 0, -1}
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
    {
        if (InitializeChunk(size)) {
            std::memcpy(fLocalPtr, data, size);
            if (ffn) {
                ffn(data, hint);
            } else {
                free(data);
            }
        }
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, UnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{size, static_cast<UnmanagedRegion*>(region.get())->fRegionId, reinterpret_cast<size_t>(hint), -1}
        , fRegionPtr(nullptr)
        , fLocalPtr(static_cast<char*>(data))
    {
        if (reinterpret_cast<const char*>(data) >= reinterpret_cast<const char*>(region->GetData()) &&
            reinterpret_cast<const char*>(data) <= reinterpret_cast<const char*>(region->GetData()) + region->GetSize()) {
            fMeta.fHandle = (boost::interprocess::managed_shared_memory::handle_t)(reinterpret_cast<const char*>(data) - reinterpret_cast<const char*>(region->GetData()));
        } else {
            LOG(error) << "trying to create region message with data from outside the region";
            throw std::runtime_error("trying to create region message with data from outside the region");
        }
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, MetaHeader& hdr, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{hdr}
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
    {
        fManager.IncrementMsgCounter();
    }

    Message(const Message&) = delete;
    Message operator=(const Message&) = delete;

    void Rebuild() override
    {
        CloseMessage();
        fQueued = false;
    }

    void Rebuild(const size_t size) override
    {
        CloseMessage();
        fQueued = false;
        InitializeChunk(size);
    }

    void Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override
    {
        CloseMessage();
        fQueued = false;

        if (InitializeChunk(size)) {
            std::memcpy(fLocalPtr, data, size);
            if (ffn) {
                ffn(data, hint);
            } else {
                free(data);
            }
        }
    }

    void* GetData() const override
    {
        if (!fLocalPtr) {
            if (fMeta.fRegionId == 0) {
                if (fMeta.fSize > 0) {
                    fLocalPtr = reinterpret_cast<char*>(fManager.Segment().get_address_from_handle(fMeta.fHandle));
                } else {
                    fLocalPtr = nullptr;
                }
            } else {
                fRegionPtr = fManager.GetRegion(fMeta.fRegionId);
                if (fRegionPtr) {
                    fLocalPtr = reinterpret_cast<char*>(fRegionPtr->fRegion.get_address()) + fMeta.fHandle;
                } else {
                    // LOG(warn) << "could not get pointer from a region message";
                    fLocalPtr = nullptr;
                }
            }
        }

        return fLocalPtr;
    }

    size_t GetSize() const override { return fMeta.fSize; }

    bool SetUsedSize(const size_t size) override
    {
        if (size == fMeta.fSize) {
            return true;
        } else if (size <= fMeta.fSize) {
            try {
                boost::interprocess::managed_shared_memory::size_type shrunkSize = size;
                fLocalPtr = fManager.Segment().allocation_command<char>(boost::interprocess::shrink_in_place, fMeta.fSize + 128, shrunkSize, fLocalPtr);
                fMeta.fSize = size;
                return true;
            } catch (boost::interprocess::interprocess_exception& e) {
                LOG(info) << "could not set used size: " << e.what();
                return false;
            }
        } else {
            LOG(error) << "cannot set used size higher than original.";
            return false;
        }
    }

    Transport GetType() const override { return fair::mq::Transport::SHM; }

    void Copy(const fair::mq::Message& msg) override
    {
        if (fMeta.fHandle < 0) {
            boost::interprocess::managed_shared_memory::handle_t otherHandle = static_cast<const Message&>(msg).fMeta.fHandle;
            if (otherHandle) {
                if (InitializeChunk(msg.GetSize())) {
                    std::memcpy(GetData(), msg.GetData(), msg.GetSize());
                }
            } else {
                LOG(error) << "copy fail: source message not initialized!";
            }
        } else {
            LOG(error) << "copy fail: target message already initialized!";
        }
    }

    ~Message() override
    {
        try {
            CloseMessage();
        } catch(SharedMemoryError& sme) {
            LOG(error) << "error closing message: " << sme.what();
        } catch(boost::interprocess::lock_exception& le) {
            LOG(error) << "error closing message: " << le.what();
        }
    }

  private:
    Manager& fManager;
    bool fQueued;
    MetaHeader fMeta;
    mutable Region* fRegionPtr;
    mutable char* fLocalPtr;

    bool InitializeChunk(const size_t size, size_t alignment = 0)
    {
        tools::RateLimiter rateLimiter(20);

        while (fMeta.fHandle < 0) {
            try {
                // boost::interprocess::managed_shared_memory::size_type actualSize = size;
                // char* hint = 0; // unused for boost::interprocess::allocate_new
                // fLocalPtr = fManager.Segment().allocation_command<char>(boost::interprocess::allocate_new, size, actualSize, hint);
                if (alignment == 0) {
                    fLocalPtr = reinterpret_cast<char*>(fManager.Segment().allocate(size));
                } else {
                    fLocalPtr = reinterpret_cast<char*>(fManager.Segment().allocate_aligned(size, alignment));
                }
            } catch (boost::interprocess::bad_alloc& ba) {
                // LOG(warn) << "Shared memory full...";
                if (fManager.ThrowingOnBadAlloc()) {
                    throw MessageBadAlloc(tools::ToString("shmem: could not create a message of size ", size, ", alignment: ", (alignment != 0) ? std::to_string(alignment) : "default"));
                }
                rateLimiter.maybe_sleep();
                if (fManager.Interrupted()) {
                    return false;
                } else {
                    continue;
                }
            }
            fMeta.fHandle = fManager.Segment().get_handle_from_address(fLocalPtr);
        }

        fMeta.fSize = size;
        return true;
    }

    void CloseMessage()
    {
        if (fMeta.fHandle >= 0 && !fQueued) {
            if (fMeta.fRegionId == 0) {
                fManager.Segment().deallocate(fManager.Segment().get_address_from_handle(fMeta.fHandle));
                fMeta.fHandle = -1;
            } else {
                if (!fRegionPtr) {
                    fRegionPtr = fManager.GetRegion(fMeta.fRegionId);
                }

                if (fRegionPtr) {
                    fRegionPtr->ReleaseBlock({fMeta.fHandle, fMeta.fSize, fMeta.fHint});
                } else {
                    LOG(warn) << "region ack queue for id " << fMeta.fRegionId << " no longer exist. Not sending ack";
                }
            }
        }

        fManager.DecrementMsgCounter();
    }
};

}
}
}

#endif /* FAIR_MQ_SHMEM_MESSAGE_H_ */

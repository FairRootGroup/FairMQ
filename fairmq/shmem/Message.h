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
#include <FairMQLogger.h>
#include <FairMQMessage.h>
#include <FairMQUnmanagedRegion.h>

#include <boost/interprocess/mapped_region.hpp>

#include <cstddef> // size_t
#include <atomic>

#include <sys/types.h> // getpid
#include <unistd.h> // pid_t

namespace fair::mq::shmem
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
        , fMeta{0, 0, 0, fManager.GetSegmentId(), -1}
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
    {
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, Alignment alignment, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{0, 0, 0, fManager.GetSegmentId(), -1}
        , fAlignment(alignment.alignment)
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
    {
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, const size_t size, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{0, 0, 0, fManager.GetSegmentId(), -1}
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
        , fMeta{0, 0, 0, fManager.GetSegmentId(), -1}
        , fAlignment(alignment.alignment)
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
    {
        InitializeChunk(size, fAlignment);
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr, FairMQTransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{0, 0, 0, fManager.GetSegmentId(), -1}
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
        , fMeta{size, reinterpret_cast<size_t>(hint), static_cast<UnmanagedRegion*>(region.get())->fRegionId, fManager.GetSegmentId(), -1}
        , fRegionPtr(nullptr)
        , fLocalPtr(static_cast<char*>(data))
    {
        if (region->GetType() != GetType()) {
            LOG(error) << "region type (" << region->GetType() << ") does not match message type (" << GetType() << ")";
            throw TransportError(tools::ToString("region type (", region->GetType(), ") does not match message type (", GetType(), ")"));
        }

        if (reinterpret_cast<const char*>(data) >= reinterpret_cast<const char*>(region->GetData()) &&
            reinterpret_cast<const char*>(data) <= reinterpret_cast<const char*>(region->GetData()) + region->GetSize()) {
            fMeta.fHandle = (boost::interprocess::managed_shared_memory::handle_t)(reinterpret_cast<const char*>(data) - reinterpret_cast<const char*>(region->GetData()));
        } else {
            LOG(error) << "trying to create region message with data from outside the region";
            throw TransportError("trying to create region message with data from outside the region");
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

    void Rebuild(Alignment alignment) override
    {
        CloseMessage();
        fQueued = false;
        fAlignment = alignment.alignment;
    }

    void Rebuild(const size_t size) override
    {
        CloseMessage();
        fQueued = false;
        InitializeChunk(size);
    }

    void Rebuild(const size_t size, Alignment alignment) override
    {
        CloseMessage();
        fQueued = false;
        fAlignment = alignment.alignment;
        InitializeChunk(size, fAlignment);
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
                    fManager.GetSegment(fMeta.fSegmentId);
                    fLocalPtr = reinterpret_cast<char*>(fManager.GetAddressFromHandle(fMeta.fHandle, fMeta.fSegmentId));
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

    bool SetUsedSize(const size_t newSize) override
    {
        if (newSize == fMeta.fSize) {
            return true;
        } else if (newSize == 0) {
            Deallocate();
            return true;
        } else if (newSize <= fMeta.fSize) {
            try {
                try {
                    fLocalPtr = fManager.ShrinkInPlace(newSize, fLocalPtr, fMeta.fSegmentId);
                    fMeta.fSize = newSize;
                    return true;
                } catch (boost::interprocess::bad_alloc& e) {
                    // if shrinking fails (can happen due to boost alignment requirements):
                    // unused size >= 1000000 bytes: reallocate fully
                    // unused size < 1000000 bytes: simply reset the size and keep the rest of the buffer until message destruction
                    if (fMeta.fSize - newSize >= 1000000) {
                        char* newPtr = fManager.Allocate(newSize, fAlignment);
                        if (newPtr) {
                            std::memcpy(newPtr, fLocalPtr, newSize);
                            fManager.Deallocate(fMeta.fHandle, fMeta.fSegmentId);
                            fLocalPtr = newPtr;
                            fMeta.fHandle = fManager.GetHandleFromAddress(fLocalPtr, fMeta.fSegmentId);
                        } else {
                            LOG(debug) << "could not set used size: " << e.what();
                            return false;
                        }
                    }
                    fMeta.fSize = newSize;
                    return true;
                }
            } catch (boost::interprocess::interprocess_exception& e) {
                LOG(debug) << "could not set used size: " << e.what();
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
    size_t fAlignment;
    mutable Region* fRegionPtr;
    mutable char* fLocalPtr;

    char* InitializeChunk(const size_t size, size_t alignment = 0)
    {
        fLocalPtr = fManager.Allocate(size, alignment);
        if (fLocalPtr) {
            fMeta.fHandle = fManager.GetHandleFromAddress(fLocalPtr, fMeta.fSegmentId);
            fMeta.fSize = size;
        }
        return fLocalPtr;
    }

    void Deallocate()
    {
        if (fMeta.fHandle >= 0 && !fQueued) {
            if (fMeta.fRegionId == 0) {
                fManager.GetSegment(fMeta.fSegmentId);
                fManager.Deallocate(fMeta.fHandle, fMeta.fSegmentId);
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
        fLocalPtr = nullptr;
        fMeta.fSize = 0;
    }

    void CloseMessage()
    {
        Deallocate();
        fAlignment = 0;

        fManager.DecrementMsgCounter();
    }
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_MESSAGE_H_ */

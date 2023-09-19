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
#include "UnmanagedRegion.h"
#include "UnmanagedRegionImpl.h"
#include <fairmq/Message.h>
#include <fairmq/UnmanagedRegion.h>

#include <fairlogger/Logger.h>

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
    Message(Manager& manager, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{0, 0, -1, -1, 0, fManager.GetSegmentId(), true}
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
    {
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, Alignment alignment, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{0, 0, -1, -1, 0, fManager.GetSegmentId(), true}
        , fAlignment(alignment.alignment)
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
    {
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, const size_t size, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{0, 0, -1, -1, 0, fManager.GetSegmentId(), true}
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
    {
        InitializeChunk(size);
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, const size_t size, Alignment alignment, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{0, 0, -1, -1, 0, fManager.GetSegmentId(), true}
        , fAlignment(alignment.alignment)
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
    {
        InitializeChunk(size, fAlignment);
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, void* data, const size_t size, fair::mq::FreeFn* ffn, void* hint = nullptr, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{0, 0, -1, -1, 0, fManager.GetSegmentId(), true}
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

    Message(Manager& manager, UnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fQueued(false)
        , fMeta{size, reinterpret_cast<size_t>(hint), -1, -1, static_cast<UnmanagedRegionImpl*>(region.get())->fRegionId, fManager.GetSegmentId(), false}
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

    Message(Manager& manager, MetaHeader& hdr, fair::mq::TransportFactory* factory = nullptr)
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
    Message(Message&&) = delete;
    Message& operator=(const Message&) = delete;
    Message& operator=(Message&&) = delete;

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

    void Rebuild(size_t size) override
    {
        CloseMessage();
        fQueued = false;
        InitializeChunk(size);
    }

    void Rebuild(size_t size, Alignment alignment) override
    {
        CloseMessage();
        fQueued = false;
        fAlignment = alignment.alignment;
        InitializeChunk(size, fAlignment);
    }

    void Rebuild(void* data, size_t size, fair::mq::FreeFn* ffn, void* hint = nullptr) override
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
            if (fMeta.fManaged) {
                if (fMeta.fSize > 0) {
                    fManager.GetSegment(fMeta.fSegmentId);
                    fLocalPtr = ShmHeader::UserPtr(fManager.GetAddressFromHandle(fMeta.fHandle, fMeta.fSegmentId));
                } else {
                    fLocalPtr = nullptr;
                }
            } else {
                fRegionPtr = fManager.GetRegionFromCache(fMeta.fRegionId);
                if (fRegionPtr) {
                    fLocalPtr = reinterpret_cast<char*>(fRegionPtr->GetData()) + fMeta.fHandle;
                } else {
                    // LOG(warn) << "could not get pointer from a region message";
                    fLocalPtr = nullptr;
                }
            }
        }

        return static_cast<void*>(fLocalPtr);
    }

    size_t GetSize() const override { return fMeta.fSize; }

    bool SetUsedSize(size_t newSize) override
    {
        if (newSize == fMeta.fSize) {
            return true;
        } else if (newSize == 0) {
            Deallocate();
            return true;
        } else if (newSize <= fMeta.fSize) {
            try {
                try {
                    char* oldPtr = fManager.GetAddressFromHandle(fMeta.fHandle, fMeta.fSegmentId);
                    uint16_t userOffset = ShmHeader::UserOffset(oldPtr);
                    char* ptr = fManager.ShrinkInPlace(userOffset + newSize, oldPtr, fMeta.fSegmentId);
                    fLocalPtr = ShmHeader::UserPtr(ptr);
                    fMeta.fSize = newSize;
                    return true;
                } catch (boost::interprocess::bad_alloc& e) {
                    // if shrinking fails (can happen due to boost alignment requirements):
                    // unused size >= 1000000 bytes: reallocate fully
                    // unused size < 1000000 bytes: simply reset the size and keep the rest of the buffer until message destruction
                    if (fMeta.fSize - newSize >= 1000000) {
                        char* ptr = fManager.Allocate(newSize, fAlignment);
                        char* userPtr = ShmHeader::UserPtr(ptr);
                        std::memcpy(userPtr, fLocalPtr, newSize);
                        fManager.Deallocate(fMeta.fHandle, fMeta.fSegmentId);
                        fLocalPtr = userPtr;
                        fMeta.fHandle = fManager.GetHandleFromAddress(ptr, fMeta.fSegmentId);
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

    uint16_t GetRefCount() const
    {
        if (fMeta.fHandle < 0) {
            return 1;
        }

        if (fMeta.fManaged) { // managed segment
            fManager.GetSegment(fMeta.fSegmentId);
            return ShmHeader::RefCount(fManager.GetAddressFromHandle(fMeta.fHandle, fMeta.fSegmentId));
        }
        if (fMeta.fShared < 0) { // UR msg is not yet shared
            return 1;
        }
        fRegionPtr = fManager.GetRegionFromCache(fMeta.fRegionId);
        if (!fRegionPtr) {
            throw TransportError(tools::ToString("Cannot get unmanaged region with id ", fMeta.fRegionId));
        }
        return fRegionPtr->GetRefCountAddressFromHandle(fMeta.fShared)->Get();
    }

    void Copy(const fair::mq::Message& other) override
    {
        const Message& otherMsg = static_cast<const Message&>(other);
        // if the other message is not initialized, close this one too and return
        if (otherMsg.fMeta.fHandle < 0) {
            CloseMessage();
            return;
        }

        // if this msg is already initialized, close it first
        if (fMeta.fHandle >= 0) {
            CloseMessage();
        }

        // increment ref count
        if (otherMsg.fMeta.fManaged) { // msg in managed segment
            fManager.GetSegment(otherMsg.fMeta.fSegmentId);
            ShmHeader::IncrementRefCount(fManager.GetAddressFromHandle(otherMsg.fMeta.fHandle, otherMsg.fMeta.fSegmentId));
        } else { // msg in unmanaged region
            fRegionPtr = fManager.GetRegionFromCache(otherMsg.fMeta.fRegionId);
            if (!fRegionPtr) {
                throw TransportError(tools::ToString("Cannot get unmanaged region with id ", otherMsg.fMeta.fRegionId));
            }
            if (otherMsg.fMeta.fShared < 0) {
                // UR msg not yet shared, create the reference counting object with count 2
                otherMsg.fMeta.fShared = fRegionPtr->HandleFromAddress(&(fRegionPtr->MakeRefCount(2)));
            } else {
                fRegionPtr->GetRefCountAddressFromHandle(otherMsg.fMeta.fShared)->Increment();
            }
        }

        // copy meta data
        fMeta = otherMsg.fMeta;
    }

    ~Message() override { CloseMessage(); }

  private:
    Manager& fManager;
    bool fQueued;
    MetaHeader fMeta;
    size_t fAlignment;
    mutable UnmanagedRegion* fRegionPtr;
    mutable char* fLocalPtr;

    char* InitializeChunk(const size_t size, size_t alignment = 0)
    {
        if (size == 0) {
            fMeta.fSize = 0;
            return fLocalPtr;
        }
        char* ptr = fManager.Allocate(size, alignment);
        fMeta.fHandle = fManager.GetHandleFromAddress(ptr, fMeta.fSegmentId);
        fMeta.fSize = size;
        fLocalPtr = ShmHeader::UserPtr(ptr);
        return fLocalPtr;
    }

    void Deallocate()
    {
        if (fMeta.fHandle >= 0 && !fQueued) {
            if (fMeta.fManaged) { // managed segment
                fManager.GetSegment(fMeta.fSegmentId);
                uint16_t refCount = ShmHeader::DecrementRefCount(fManager.GetAddressFromHandle(fMeta.fHandle, fMeta.fSegmentId));
                if (refCount == 1) {
                    fManager.Deallocate(fMeta.fHandle, fMeta.fSegmentId);
                }
            } else { // unmanaged region
                if (fMeta.fShared >= 0) {
                    fRegionPtr = fManager.GetRegionFromCache(fMeta.fRegionId);
                    if (!fRegionPtr) {
                        throw TransportError(tools::ToString("Cannot get unmanaged region with id ", fMeta.fRegionId));
                    }
                    uint16_t refCount = fRegionPtr->GetRefCountAddressFromHandle(fMeta.fShared)->Decrement();
                    if (refCount == 1) {
                        fRegionPtr->RemoveRefCount(*(fRegionPtr->GetRefCountAddressFromHandle(fMeta.fShared)));
                        ReleaseUnmanagedRegionBlock();
                    }
                } else {
                    ReleaseUnmanagedRegionBlock();
                }
            }
        }
        fMeta.fHandle = -1;
        fLocalPtr = nullptr;
        fMeta.fSize = 0;
    }

    void ReleaseUnmanagedRegionBlock()
    {
        if (!fRegionPtr) {
            fRegionPtr = fManager.GetRegionFromCache(fMeta.fRegionId);
        }

        if (fRegionPtr) {
            fRegionPtr->ReleaseBlock({fMeta.fHandle, fMeta.fSize, fMeta.fHint});
        } else {
            LOG(warn) << "region ack queue for id " << fMeta.fRegionId << " no longer exist. Not sending ack";
        }
    }

    void CloseMessage()
    {
        try {
            Deallocate();
            fAlignment = 0;
            fManager.DecrementMsgCounter();
        } catch (SharedMemoryError& sme) {
            LOG(error) << "error closing message: " << sme.what();
        } catch (boost::interprocess::lock_exception& le) {
            LOG(error) << "error closing message: " << le.what();
        }
    }
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_MESSAGE_H_ */

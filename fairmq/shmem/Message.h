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
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
        , fSize(0)
        , fHint(0)
        , fHandle(-1)
        , fShared(-1)
        , fRegionId(0)
        , fSegmentId(fManager.GetSegmentId())
        , fAlignment(0)
        , fManaged(true)
        , fQueued(false)
    {
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, Alignment alignment, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
        , fSize(0)
        , fHint(0)
        , fHandle(-1)
        , fShared(-1)
        , fRegionId(0)
        , fSegmentId(fManager.GetSegmentId())
        , fAlignment(alignment.alignment)
        , fManaged(true)
        , fQueued(false)
    {
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, const size_t size, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
        , fSize(0)
        , fHint(0)
        , fHandle(-1)
        , fShared(-1)
        , fRegionId(0)
        , fSegmentId(fManager.GetSegmentId())
        , fAlignment(0)
        , fManaged(true)
        , fQueued(false)
    {
        InitializeChunk(size);
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, const size_t size, Alignment alignment, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
        , fSize(0)
        , fHint(0)
        , fHandle(-1)
        , fShared(-1)
        , fRegionId(0)
        , fSegmentId(fManager.GetSegmentId())
        , fAlignment(alignment.alignment)
        , fManaged(true)
        , fQueued(false)
    {
        InitializeChunk(size, fAlignment);
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, void* data, const size_t size, fair::mq::FreeFn* ffn, void* hint = nullptr, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
        , fSize(0)
        , fHint(0)
        , fHandle(-1)
        , fShared(-1)
        , fRegionId(0)
        , fSegmentId(fManager.GetSegmentId())
        , fAlignment(0)
        , fManaged(true)
        , fQueued(false)
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
        , fRegionPtr(nullptr)
        , fLocalPtr(static_cast<char*>(data))
        , fSize(size)
        , fHint(reinterpret_cast<size_t>(hint))
        , fHandle(-1)
        , fShared(-1)
        , fRegionId(static_cast<UnmanagedRegionImpl*>(region.get())->fRegionId)
        , fSegmentId(fManager.GetSegmentId())
        , fAlignment(0)
        , fManaged(false)
        , fQueued(false)
    {
        if (region->GetType() != GetType()) {
            LOG(error) << "region type (" << region->GetType() << ") does not match message type (" << GetType() << ")";
            throw TransportError(tools::ToString("region type (", region->GetType(), ") does not match message type (", GetType(), ")"));
        }

        if (reinterpret_cast<const char*>(data) >= reinterpret_cast<const char*>(region->GetData()) &&
            reinterpret_cast<const char*>(data) <= reinterpret_cast<const char*>(region->GetData()) + region->GetSize()) {
            fHandle = (boost::interprocess::managed_shared_memory::handle_t)(reinterpret_cast<const char*>(data) - reinterpret_cast<const char*>(region->GetData()));
        } else {
            LOG(error) << "trying to create region message with data from outside the region";
            throw TransportError("trying to create region message with data from outside the region");
        }
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, MetaHeader& hdr, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fRegionPtr(nullptr)
        , fLocalPtr(nullptr)
        , fSize(hdr.fSize)
        , fHint(hdr.fHint)
        , fHandle(hdr.fHandle)
        , fShared(hdr.fShared)
        , fRegionId(hdr.fRegionId)
        , fSegmentId(hdr.fSegmentId)
        , fAlignment(0)
        , fManaged(hdr.fManaged)
        , fQueued(false)
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
            if (fManaged) {
                if (fSize > 0) {
                    fManager.GetSegment(fSegmentId);
                    fLocalPtr = ShmHeader::UserPtr(fManager.GetAddressFromHandle(fHandle, fSegmentId));
                } else {
                    fLocalPtr = nullptr;
                }
            } else {
                fRegionPtr = fManager.GetRegionFromCache(fRegionId);
                if (fRegionPtr) {
                    fLocalPtr = reinterpret_cast<char*>(fRegionPtr->GetData()) + fHandle;
                } else {
                    // LOG(warn) << "could not get pointer from a region message";
                    fLocalPtr = nullptr;
                }
            }
        }

        return static_cast<void*>(fLocalPtr);
    }

    size_t GetSize() const override { return fSize; }

    bool SetUsedSize(size_t newSize) override
    {
        if (newSize == fSize) {
            return true;
        } else if (newSize == 0) {
            Deallocate();
            return true;
        } else if (newSize <= fSize) {
            try {
                try {
                    char* oldPtr = fManager.GetAddressFromHandle(fHandle, fSegmentId);
                    uint16_t userOffset = ShmHeader::UserOffset(oldPtr);
                    char* ptr = fManager.ShrinkInPlace(userOffset + newSize, oldPtr, fSegmentId);
                    fLocalPtr = ShmHeader::UserPtr(ptr);
                    fSize = newSize;
                    return true;
                } catch (boost::interprocess::bad_alloc& e) {
                    // if shrinking fails (can happen due to boost alignment requirements):
                    // unused size >= 1000000 bytes: reallocate fully
                    // unused size < 1000000 bytes: simply reset the size and keep the rest of the buffer until message destruction
                    if (fSize - newSize >= 1000000) {
                        char* ptr = fManager.Allocate(newSize, fAlignment);
                        char* userPtr = ShmHeader::UserPtr(ptr);
                        std::memcpy(userPtr, fLocalPtr, newSize);
                        fManager.Deallocate(fHandle, fSegmentId);
                        fLocalPtr = userPtr;
                        fHandle = fManager.GetHandleFromAddress(ptr, fSegmentId);
                    }
                    fSize = newSize;
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
        if (fHandle < 0) {
            return 1;
        }

        if (fManaged) { // managed segment
            fManager.GetSegment(fSegmentId);
            return ShmHeader::RefCount(fManager.GetAddressFromHandle(fHandle, fSegmentId));
        }
        if (fShared < 0) { // UR msg is not yet shared
            return 1;
        }
        fRegionPtr = fManager.GetRegionFromCache(fRegionId);
        if (!fRegionPtr) {
            throw TransportError(tools::ToString("Cannot get unmanaged region with id ", fRegionId));
        }
        return fRegionPtr->GetRefCountAddressFromHandle(fShared)->Get();
    }

    void Copy(const fair::mq::Message& other) override
    {
        const Message& otherMsg = static_cast<const Message&>(other);
        // if the other message is not initialized, close this one too and return
        if (otherMsg.fHandle < 0) {
            CloseMessage();
            return;
        }

        // if this msg is already initialized, close it first
        if (fHandle >= 0) {
            CloseMessage();
        }

        // increment ref count
        if (otherMsg.fManaged) { // msg in managed segment
            fManager.GetSegment(otherMsg.fSegmentId);
            ShmHeader::IncrementRefCount(fManager.GetAddressFromHandle(otherMsg.fHandle, otherMsg.fSegmentId));
        } else { // msg in unmanaged region
            fRegionPtr = fManager.GetRegionFromCache(otherMsg.fRegionId);
            if (!fRegionPtr) {
                throw TransportError(tools::ToString("Cannot get unmanaged region with id ", otherMsg.fRegionId));
            }
            if (otherMsg.fShared < 0) {
                // UR msg not yet shared, create the reference counting object with count 2
                otherMsg.fShared = fRegionPtr->HandleFromAddress(&(fRegionPtr->MakeRefCount(2)));
            } else {
                fRegionPtr->GetRefCountAddressFromHandle(otherMsg.fShared)->Increment();
            }
        }

        // copy meta data
        fSize = otherMsg.fSize;
        fHint = otherMsg.fHint;
        fHandle = otherMsg.fHandle;
        fShared = otherMsg.fShared;
        fRegionId = otherMsg.fRegionId;
        fSegmentId = otherMsg.fSegmentId;
        fManaged = otherMsg.fManaged;
    }

    ~Message() override { CloseMessage(); }

  private:
    Manager& fManager;
    mutable UnmanagedRegion* fRegionPtr;
    mutable char* fLocalPtr;
    size_t fSize; // size of the shm buffer
    size_t fHint; // user-defined value, given by the user on message creation and returned to the user on "buffer no longer needed"-callbacks
    boost::interprocess::managed_shared_memory::handle_t fHandle; // handle to shm buffer, convertible to shm buffer ptr
    mutable boost::interprocess::managed_shared_memory::handle_t fShared; // handle to the buffer storing the ref count for shared buffers
    uint16_t fRegionId; // id of the unmanaged region
    mutable uint16_t fSegmentId; // id of the managed segment
    size_t fAlignment;
    bool fManaged; // true = managed segment, false = unmanaged region
    bool fQueued;

    void SetMeta(const MetaHeader& meta)
    {
        fSize = meta.fSize;
        fHint = meta.fHint;
        fHandle = meta.fHandle;
        fShared = meta.fShared;
        fRegionId = meta.fRegionId;
        fSegmentId = meta.fSegmentId;
        fManaged = meta.fManaged;
    }

    char* InitializeChunk(const size_t size, size_t alignment = 0)
    {
        if (size == 0) {
            fSize = 0;
            return fLocalPtr;
        }
        char* ptr = fManager.Allocate(size, alignment);
        fHandle = fManager.GetHandleFromAddress(ptr, fSegmentId);
        fSize = size;
        fLocalPtr = ShmHeader::UserPtr(ptr);
        return fLocalPtr;
    }

    void Deallocate()
    {
        if (fHandle >= 0 && !fQueued) {
            if (fManaged) { // managed segment
                fManager.GetSegment(fSegmentId);
                uint16_t refCount = ShmHeader::DecrementRefCount(fManager.GetAddressFromHandle(fHandle, fSegmentId));
                if (refCount == 1) {
                    fManager.Deallocate(fHandle, fSegmentId);
                }
            } else { // unmanaged region
                if (fShared >= 0) {
                    fRegionPtr = fManager.GetRegionFromCache(fRegionId);
                    if (!fRegionPtr) {
                        throw TransportError(tools::ToString("Cannot get unmanaged region with id ", fRegionId));
                    }
                    uint16_t refCount = fRegionPtr->GetRefCountAddressFromHandle(fShared)->Decrement();
                    if (refCount == 1) {
                        fRegionPtr->RemoveRefCount(*(fRegionPtr->GetRefCountAddressFromHandle(fShared)));
                        ReleaseUnmanagedRegionBlock();
                    }
                } else {
                    ReleaseUnmanagedRegionBlock();
                }
            }
        }
        fHandle = -1;
        fLocalPtr = nullptr;
        fSize = 0;
    }

    void ReleaseUnmanagedRegionBlock()
    {
        if (!fRegionPtr) {
            fRegionPtr = fManager.GetRegionFromCache(fRegionId);
        }

        if (fRegionPtr) {
            fRegionPtr->ReleaseBlock({fHandle, fSize, fHint});
        } else {
            LOG(warn) << "region ack queue for id " << fRegionId << " no longer exist. Not sending ack";
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

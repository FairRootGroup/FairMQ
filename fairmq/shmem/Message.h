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
        : Message(manager, Alignment{0}, factory)
    {}

    Message(Manager& manager, Alignment /* alignment */, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fSegmentId(fManager.GetSegmentId())
    {
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, const size_t size, fair::mq::TransportFactory* factory = nullptr)
        : Message(manager, size, Alignment{0}, factory)
    {}

    Message(Manager& manager, const size_t size, Alignment alignment, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fSegmentId(fManager.GetSegmentId())
    {
        InitializeChunk(size, alignment.alignment);
        fManager.IncrementMsgCounter();
    }

    Message(Manager& manager, void* data, const size_t size, fair::mq::FreeFn* ffn, void* hint = nullptr, fair::mq::TransportFactory* factory = nullptr)
        : fair::mq::Message(factory)
        , fManager(manager)
        , fSegmentId(fManager.GetSegmentId())
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
        , fLocalPtr(static_cast<char*>(data))
        , fSize(size)
        , fHint(reinterpret_cast<size_t>(hint))
        , fRegionId(static_cast<UnmanagedRegionImpl*>(region.get())->fRegionId)
        , fSegmentId(fManager.GetSegmentId())
        , fManaged(false)
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
        , fSize(hdr.fSize)
        , fHint(hdr.fHint)
        , fHandle(hdr.fHandle)
        , fShared(hdr.fShared)
        , fRegionId(hdr.fRegionId)
        , fSegmentId(hdr.fSegmentId)
        , fManaged(hdr.fManaged)
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

    void Rebuild(Alignment /* alignment */) override
    {
        CloseMessage();
        fQueued = false;
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
        InitializeChunk(size, alignment.alignment);
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

    bool SetUsedSize(size_t newSize, Alignment alignment = Alignment{0}) override
    {
        if (newSize == fSize) {
            return true;
        } else if (newSize == 0) {
            Deallocate();
            return true;
        } else if (newSize <= fSize) {
            try {
                char* oldPtr = fManager.GetAddressFromHandle(fHandle, fSegmentId);
                try {
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
                        if (alignment.alignment == 0) {
                            // if no alignment is provided, take the minimum alignment of the old pointer, but no more than 4096
                            alignment.alignment = 1 << std::min(__builtin_ctz(reinterpret_cast<size_t>(oldPtr)), 12);
                        }
                        char* ptr = fManager.Allocate(newSize, alignment.alignment);
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
        if (fRegionPtr->fRcSegmentSize > 0) {
            return fRegionPtr->GetRefCountAddressFromHandle(fShared)->Get();
        } else {
            fManager.GetSegment(fSegmentId);
            return ShmHeader::RefCount(fManager.GetAddressFromHandle(fShared, fSegmentId));
        }
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
            if (fRegionPtr->fRcSegmentSize > 0) {
                if (otherMsg.fShared < 0) {
                    // UR msg not yet shared, create the reference counting object with count 2
                    try {
                        otherMsg.fShared = fRegionPtr->HandleFromAddress(&(fRegionPtr->MakeRefCount(2)));
                    } catch (boost::interprocess::bad_alloc& ba) {
                        throw RefCountBadAlloc(tools::ToString("Insufficient space in the reference count segment ", otherMsg.fRegionId, ", original exception: bad_alloc: ", ba.what()));
                    }
                } else {
                    fRegionPtr->GetRefCountAddressFromHandle(otherMsg.fShared)->Increment();
                }
            } else { // if RefCount segment size is 0, store the ref count in the managed segment
                if (otherMsg.fShared < 0) { // if UR msg is not yet shared
                    char* ptr = fManager.Allocate(2, 0);
                    // point the fShared in the unmanaged region message to the refCount holder
                    otherMsg.fShared = fManager.GetHandleFromAddress(ptr, fSegmentId);
                    // the message needs to be able to locate in which segment the refCount is stored
                    otherMsg.fSegmentId = fSegmentId;
                    ShmHeader::IncrementRefCount(ptr);
                } else { // if the UR msg is already shared
                    fManager.GetSegment(otherMsg.fSegmentId);
                    ShmHeader::IncrementRefCount(fManager.GetAddressFromHandle(otherMsg.fShared, otherMsg.fSegmentId));
                }
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
    mutable UnmanagedRegion* fRegionPtr = nullptr;
    mutable char* fLocalPtr = nullptr;
    size_t fSize = 0; // size of the shm buffer
    size_t fHint = 0; // user-defined value, given by the user on message creation and returned to the user on "buffer no longer needed"-callbacks
    boost::interprocess::managed_shared_memory::handle_t fHandle = -1; // handle to shm buffer, convertible to shm buffer ptr
    mutable boost::interprocess::managed_shared_memory::handle_t fShared = -1; // handle to the buffer storing the ref count for shared buffers
    uint16_t fRegionId = 0; // id of the unmanaged region
    mutable uint16_t fSegmentId; // id of the managed segment
    bool fManaged = true; // true = managed segment, false = unmanaged region
    bool fQueued = false;

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
                    if (fRegionPtr->fRcSegmentSize > 0) {
                        uint16_t refCount = fRegionPtr->GetRefCountAddressFromHandle(fShared)->Decrement();
                        if (refCount == 1) {
                            fRegionPtr->RemoveRefCount(*(fRegionPtr->GetRefCountAddressFromHandle(fShared)));
                            ReleaseUnmanagedRegionBlock();
                        }
                    } else { // if RefCount segment size is 0, get the ref count from the managed segment
                        // make sure segment is initialized in this transport
                        fManager.GetSegment(fSegmentId);
                        // release unmanaged region block if ref count is one
                        uint16_t refCount = ShmHeader::DecrementRefCount(fManager.GetAddressFromHandle(fShared, fSegmentId));
                        if (refCount == 1) {
                            fManager.Deallocate(fShared, fSegmentId);
                            ReleaseUnmanagedRegionBlock();
                        }
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

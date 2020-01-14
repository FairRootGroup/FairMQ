/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Region.h"
#include "Message.h"
#include "UnmanagedRegion.h"
#include "TransportFactory.h"

#include <FairMQLogger.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <cstdlib>

using namespace std;

namespace bipc = ::boost::interprocess;
namespace bpt = ::boost::posix_time;

namespace fair
{
namespace mq
{
namespace shmem
{

atomic<bool> Message::fInterrupted(false);
Transport Message::fTransportType = Transport::SHM;

Message::Message(Manager& manager, FairMQTransportFactory* factory)
    : fair::mq::Message{factory}
    , fManager(manager)
    , fQueued(false)
    , fMeta{0, 0, 0, -1}
    , fRegionPtr(nullptr)
    , fLocalPtr(nullptr)
{
    static_cast<TransportFactory*>(GetTransport())->IncrementMsgCounter();
}

Message::Message(Manager& manager, const size_t size, FairMQTransportFactory* factory)
    : fair::mq::Message{factory}
    , fManager(manager)
    , fQueued(false)
    , fMeta{0, 0, 0, -1}
    , fRegionPtr(nullptr)
    , fLocalPtr(nullptr)
{
    InitializeChunk(size);
    static_cast<TransportFactory*>(GetTransport())->IncrementMsgCounter();
}

Message::Message(Manager& manager, MetaHeader& hdr, FairMQTransportFactory* factory)
    : fair::mq::Message{factory}
    , fManager(manager)
    , fQueued(false)
    , fMeta{hdr}
    , fRegionPtr(nullptr)
    , fLocalPtr(nullptr)
{
    static_cast<TransportFactory*>(GetTransport())->IncrementMsgCounter();
}

Message::Message(Manager& manager, void* data, const size_t size, fairmq_free_fn* ffn, void* hint, FairMQTransportFactory* factory)
    : fair::mq::Message{factory}
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
    static_cast<TransportFactory*>(GetTransport())->IncrementMsgCounter();
}

Message::Message(Manager& manager, UnmanagedRegionPtr& region, void* data, const size_t size, void* hint, FairMQTransportFactory* factory)
    : fair::mq::Message{factory}
    , fManager(manager)
    , fQueued(false)
    , fMeta{size, static_cast<UnmanagedRegion*>(region.get())->fRegionId, reinterpret_cast<size_t>(hint), -1}
    , fRegionPtr(nullptr)
    , fLocalPtr(static_cast<char*>(data))
{
    if (reinterpret_cast<const char*>(data) >= reinterpret_cast<const char*>(region->GetData()) ||
        reinterpret_cast<const char*>(data) <= reinterpret_cast<const char*>(region->GetData()) + region->GetSize()) {
        fMeta.fHandle = (bipc::managed_shared_memory::handle_t)(reinterpret_cast<const char*>(data) - reinterpret_cast<const char*>(region->GetData()));
    } else {
        LOG(error) << "trying to create region message with data from outside the region";
        throw runtime_error("trying to create region message with data from outside the region");
    }
    static_cast<TransportFactory*>(GetTransport())->IncrementMsgCounter();
}

bool Message::InitializeChunk(const size_t size)
{
    while (fMeta.fHandle < 0) {
        try {
            bipc::managed_shared_memory::size_type actualSize = size;
            char* hint = 0; // unused for bipc::allocate_new
            fLocalPtr = fManager.Segment().allocation_command<char>(bipc::allocate_new, size, actualSize, hint);
        } catch (bipc::bad_alloc& ba) {
            // LOG(warn) << "Shared memory full...";
            this_thread::sleep_for(chrono::milliseconds(50));
            if (fInterrupted) {
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

void Message::Rebuild()
{
    CloseMessage();
    fQueued = false;
}

void Message::Rebuild(const size_t size)
{
    CloseMessage();
    fQueued = false;
    InitializeChunk(size);
}

void Message::Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint)
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

void* Message::GetData() const
{
    if (!fLocalPtr) {
        if (fMeta.fRegionId == 0) {
            if (fMeta.fSize > 0) {
                fLocalPtr = reinterpret_cast<char*>(fManager.Segment().get_address_from_handle(fMeta.fHandle));
            } else {
                fLocalPtr = nullptr;
            }
        } else {
            fRegionPtr = fManager.GetRemoteRegion(fMeta.fRegionId);
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

bool Message::SetUsedSize(const size_t size)
{
    if (size == fMeta.fSize) {
        return true;
    } else if (size <= fMeta.fSize) {
        try {
            bipc::managed_shared_memory::size_type shrunkSize = size;
            fLocalPtr = fManager.Segment().allocation_command<char>(bipc::shrink_in_place, fMeta.fSize + 128, shrunkSize, fLocalPtr);
            fMeta.fSize = size;
            return true;
        } catch (bipc::interprocess_exception& e) {
            LOG(info) << "could not set used size: " << e.what();
            return false;
        }
    } else {
        LOG(error) << "cannot set used size higher than original.";
        return false;
    }
}

void Message::Copy(const fair::mq::Message& msg)
{
    if (fMeta.fHandle < 0) {
        bipc::managed_shared_memory::handle_t otherHandle = static_cast<const Message&>(msg).fMeta.fHandle;
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

void Message::CloseMessage()
{
    if (fMeta.fHandle >= 0 && !fQueued) {
        if (fMeta.fRegionId == 0) {
            fManager.Segment().deallocate(fManager.Segment().get_address_from_handle(fMeta.fHandle));
            fMeta.fHandle = -1;
        } else {
            if (!fRegionPtr) {
                fRegionPtr = fManager.GetRemoteRegion(fMeta.fRegionId);
            }

            if (fRegionPtr) {
                fRegionPtr->ReleaseBlock({fMeta.fHandle, fMeta.fSize, fMeta.fHint});
            } else {
                LOG(warn) << "region ack queue for id " << fMeta.fRegionId << " no longer exist. Not sending ack";
            }
        }
    }

    static_cast<TransportFactory*>(GetTransport())->DecrementMsgCounter();
}

Message::~Message()
{
    try {
        CloseMessage();
    } catch(SharedMemoryError& sme) {
        LOG(error) << "error closing message: " << sme.what();
    } catch(bipc::lock_exception& le) {
        LOG(error) << "error closing message: " << le.what();
    }
}

}
}
}

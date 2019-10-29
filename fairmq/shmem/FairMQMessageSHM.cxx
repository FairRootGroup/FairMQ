/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "Common.h"
#include "Region.h"

#include "FairMQMessageSHM.h"
#include "FairMQUnmanagedRegionSHM.h"
#include "FairMQLogger.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <cstdlib>

using namespace std;
using namespace fair::mq::shmem;

namespace bipc = ::boost::interprocess;
namespace bpt = ::boost::posix_time;

atomic<bool> FairMQMessageSHM::fInterrupted(false);
fair::mq::Transport FairMQMessageSHM::fTransportType = fair::mq::Transport::SHM;

FairMQMessageSHM::FairMQMessageSHM(Manager& manager, FairMQTransportFactory* factory)
    : FairMQMessage{factory}
    , fManager(manager)
    , fMessage()
    , fQueued(false)
    , fMetaCreated(false)
    , fRegionId(0)
    , fRegionPtr(nullptr)
    , fHandle(-1)
    , fSize(0)
    , fHint(0)
    , fLocalPtr(nullptr)
{
    if (zmq_msg_init(&fMessage) != 0) {
        LOG(error) << "failed initializing message, reason: " << zmq_strerror(errno);
    }
    fMetaCreated = true;
}

FairMQMessageSHM::FairMQMessageSHM(Manager& manager, const size_t size, FairMQTransportFactory* factory)
    : FairMQMessage{factory}
    , fManager(manager)
    , fMessage()
    , fQueued(false)
    , fMetaCreated(false)
    , fRegionId(0)
    , fRegionPtr(nullptr)
    , fHandle(-1)
    , fSize(0)
    , fHint(0)
    , fLocalPtr(nullptr)
{
    InitializeChunk(size);
}

FairMQMessageSHM::FairMQMessageSHM(Manager& manager, void* data, const size_t size, fairmq_free_fn* ffn, void* hint, FairMQTransportFactory* factory)
    : FairMQMessage{factory}
    , fManager(manager)
    , fMessage()
    , fQueued(false)
    , fMetaCreated(false)
    , fRegionId(0)
    , fRegionPtr(nullptr)
    , fHandle(-1)
    , fSize(0)
    , fHint(0)
    , fLocalPtr(nullptr)
{
    if (InitializeChunk(size)) {
        memcpy(fLocalPtr, data, size);
        if (ffn) {
            ffn(data, hint);
        } else {
            free(data);
        }
    }
}

FairMQMessageSHM::FairMQMessageSHM(Manager& manager, FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint, FairMQTransportFactory* factory)
    : FairMQMessage{factory}
    , fManager(manager)
    , fMessage()
    , fQueued(false)
    , fMetaCreated(false)
    , fRegionId(static_cast<FairMQUnmanagedRegionSHM*>(region.get())->fRegionId)
    , fRegionPtr(nullptr)
    , fHandle(-1)
    , fSize(size)
    , fHint(reinterpret_cast<size_t>(hint))
    , fLocalPtr(static_cast<char*>(data))
{
    if (reinterpret_cast<const char*>(data) >= reinterpret_cast<const char*>(region->GetData()) ||
        reinterpret_cast<const char*>(data) <= reinterpret_cast<const char*>(region->GetData()) + region->GetSize()) {
        fHandle = (bipc::managed_shared_memory::handle_t)(reinterpret_cast<const char*>(data) - reinterpret_cast<const char*>(region->GetData()));

        if (zmq_msg_init_size(&fMessage, sizeof(MetaHeader)) != 0) {
            LOG(error) << "failed initializing meta message, reason: " << zmq_strerror(errno);
        } else {
            MetaHeader header;
            header.fSize = size;
            header.fHandle = fHandle;
            header.fRegionId = fRegionId;
            header.fHint = fHint;
            memcpy(zmq_msg_data(&fMessage), &header, sizeof(MetaHeader));

            fMetaCreated = true;
        }
    } else {
        LOG(error) << "trying to create region message with data from outside the region";
        throw runtime_error("trying to create region message with data from outside the region");
    }
}

bool FairMQMessageSHM::InitializeChunk(const size_t size)
{
    while (fHandle < 0) {
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
        fHandle = fManager.Segment().get_handle_from_address(fLocalPtr);
    }

    fSize = size;

    if (zmq_msg_init_size(&fMessage, sizeof(MetaHeader)) != 0) {
        LOG(error) << "failed initializing meta message, reason: " << zmq_strerror(errno);
        return false;
    }
    MetaHeader header;
    header.fSize = size;
    header.fHandle = fHandle;
    header.fRegionId = fRegionId;
    header.fHint = fHint;
    memcpy(zmq_msg_data(&fMessage), &header, sizeof(MetaHeader));

    fMetaCreated = true;

    return true;
}

void FairMQMessageSHM::Rebuild()
{
    CloseMessage();

    fQueued = false;

    if (zmq_msg_init(&fMessage) != 0) {
        LOG(error) << "failed initializing message, reason: " << zmq_strerror(errno);
    }
    fMetaCreated = true;
}

void FairMQMessageSHM::Rebuild(const size_t size)
{
    CloseMessage();
    fQueued = false;
    InitializeChunk(size);
}

void FairMQMessageSHM::Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint)
{
    CloseMessage();

    fQueued = false;

    if (InitializeChunk(size)) {
        memcpy(fLocalPtr, data, size);
        if (ffn) {
            ffn(data, hint);
        } else {
            free(data);
        }
    }
}

void* FairMQMessageSHM::GetData() const
{
    if (!fLocalPtr) {
        if (fRegionId == 0) {
            if (fSize > 0) {
                fLocalPtr = reinterpret_cast<char*>(fManager.Segment().get_address_from_handle(fHandle));
            } else {
                fLocalPtr = nullptr;
            }
        } else {
            fRegionPtr = fManager.GetRemoteRegion(fRegionId);
            if (fRegionPtr) {
                fLocalPtr = reinterpret_cast<char*>(fRegionPtr->fRegion.get_address()) + fHandle;
            } else {
                // LOG(warn) << "could not get pointer from a region message";
                fLocalPtr = nullptr;
            }
        }
    }

    return fLocalPtr;
}

bool FairMQMessageSHM::SetUsedSize(const size_t size)
{
    if (size == fSize) {
        return true;
    } else if (size <= fSize) {
        try {
            bipc::managed_shared_memory::size_type shrunkSize = size;
            fLocalPtr = fManager.Segment().allocation_command<char>(bipc::shrink_in_place, fSize + 128, shrunkSize, fLocalPtr);
            fSize = size;

            // update meta header
            MetaHeader* hdrPtr = static_cast<MetaHeader*>(zmq_msg_data(&fMessage));
            hdrPtr->fSize = fSize;
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

void FairMQMessageSHM::Copy(const FairMQMessage& msg)
{
    if (fHandle < 0) {
        bipc::managed_shared_memory::handle_t otherHandle = static_cast<const FairMQMessageSHM&>(msg).fHandle;
        if (otherHandle) {
            if (InitializeChunk(msg.GetSize())) {
                memcpy(GetData(), msg.GetData(), msg.GetSize());
            }
        } else {
            LOG(error) << "copy fail: source message not initialized!";
        }
    } else {
        LOG(error) << "copy fail: target message already initialized!";
    }
}

void FairMQMessageSHM::CloseMessage()
{
    if (fHandle >= 0 && !fQueued) {
        if (fRegionId == 0) {
            fManager.Segment().deallocate(fManager.Segment().get_address_from_handle(fHandle));
            fHandle = -1;
        } else {
            if (!fRegionPtr) {
                fRegionPtr = fManager.GetRemoteRegion(fRegionId);
            }

            if (fRegionPtr) {
                fRegionPtr->ReleaseBlock({fHandle, fSize, fHint});
            } else {
                LOG(warn) << "region ack queue for id " << fRegionId << " no longer exist. Not sending ack";
            }
        }
    }

    if (fMetaCreated) {
        if (zmq_msg_close(&fMessage) != 0) {
            LOG(error) << "failed closing message, reason: " << zmq_strerror(errno);
        }
        fMetaCreated = false;
    }
}

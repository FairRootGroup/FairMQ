/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include <fairmq/shmem/Common.h>
#include <fairmq/shmem/Region.h>

#include "FairMQMessageSHM.h"
#include "FairMQUnmanagedRegionSHM.h"
#include "FairMQLogger.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <cstdlib>

using namespace std;
using namespace fair::mq::shmem;

namespace bipc = boost::interprocess;
namespace bpt = boost::posix_time;

atomic<bool> FairMQMessageSHM::fInterrupted(false);
FairMQ::Transport FairMQMessageSHM::fTransportType = FairMQ::Transport::SHM;

FairMQMessageSHM::FairMQMessageSHM(Manager& manager)
    : fManager(manager)
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
    if (zmq_msg_init(&fMessage) != 0)
    {
        LOG(ERROR) << "failed initializing message, reason: " << zmq_strerror(errno);
    }
    fMetaCreated = true;
}

FairMQMessageSHM::FairMQMessageSHM(Manager& manager, const size_t size)
    : fManager(manager)
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

FairMQMessageSHM::FairMQMessageSHM(Manager& manager, void* data, const size_t size, fairmq_free_fn* ffn, void* hint)
    : fManager(manager)
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
    if (InitializeChunk(size))
    {
        memcpy(fLocalPtr, data, size);
        if (ffn)
        {
            ffn(data, hint);
        }
        else
        {
            free(data);
        }
    }
}

FairMQMessageSHM::FairMQMessageSHM(Manager& manager, FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint)
    : fManager(manager)
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
        reinterpret_cast<const char*>(data) <= reinterpret_cast<const char*>(region->GetData()) + region->GetSize())
    {
        fHandle = (bipc::managed_shared_memory::handle_t)(reinterpret_cast<const char*>(data) - reinterpret_cast<const char*>(region->GetData()));

        if (zmq_msg_init_size(&fMessage, sizeof(MetaHeader)) != 0)
        {
            LOG(ERROR) << "failed initializing meta message, reason: " << zmq_strerror(errno);
        }
        else
        {
            MetaHeader header;
            header.fSize = size;
            header.fHandle = fHandle;
            header.fRegionId = fRegionId;
            header.fHint = fHint;
            memcpy(zmq_msg_data(&fMessage), &header, sizeof(MetaHeader));

            fMetaCreated = true;
        }
    }
    else
    {
        LOG(ERROR) << "shmem: trying to create region message with data from outside the region";
        throw runtime_error("shmem: trying to create region message with data from outside the region");
    }
}

bool FairMQMessageSHM::InitializeChunk(const size_t size)
{
    while (fHandle < 0)
    {
        try
        {
            bipc::managed_shared_memory::size_type actualSize = size;
            char* hint = 0; // unused for bipc::allocate_new
            fLocalPtr = fManager.Segment().allocation_command<char>(bipc::allocate_new, size, actualSize, hint);
        }
        catch (bipc::bad_alloc& ba)
        {
            // LOG(WARN) << "Shared memory full...";
            this_thread::sleep_for(chrono::milliseconds(50));
            if (fInterrupted)
            {
                return false;
            }
            else
            {
                continue;
            }
        }
        fHandle = fManager.Segment().get_handle_from_address(fLocalPtr);
    }

    fSize = size;

    if (zmq_msg_init_size(&fMessage, sizeof(MetaHeader)) != 0)
    {
        LOG(ERROR) << "failed initializing meta message, reason: " << zmq_strerror(errno);
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

    if (zmq_msg_init(&fMessage) != 0)
    {
        LOG(ERROR) << "failed initializing message, reason: " << zmq_strerror(errno);
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

    if (InitializeChunk(size))
    {
        memcpy(fLocalPtr, data, size);
        if (ffn)
        {
            ffn(data, hint);
        }
        else
        {
            free(data);
        }
    }
}

zmq_msg_t* FairMQMessageSHM::GetMessage()
{
    return &fMessage;
}

void* FairMQMessageSHM::GetData() const
{
    if (fLocalPtr)
    {
        return fLocalPtr;
    }
    else
    {
        if (fRegionId == 0)
        {
            return fManager.Segment().get_address_from_handle(fHandle);
        }
        else
        {
            fRegionPtr = fManager.GetRemoteRegion(fRegionId);
            if (fRegionPtr)
            {
                fLocalPtr = reinterpret_cast<char*>(fRegionPtr->fRegion.get_address()) + fHandle;
            }
            else
            {
                // LOG(WARN) << "could not get pointer from a region message";
                fLocalPtr = nullptr;
            }
            return fLocalPtr;
        }
    }
}

size_t FairMQMessageSHM::GetSize() const
{
    return fSize;
}

bool FairMQMessageSHM::SetUsedSize(const size_t size)
{
    if (size == fSize)
    {
        return true;
    }
    else if (size <= fSize)
    {
        try
        {

            bipc::managed_shared_memory::size_type shrunkSize = size;
            fLocalPtr = fManager.Segment().allocation_command<char>(bipc::shrink_in_place, fSize + 128, shrunkSize, fLocalPtr);
            fSize = size;

            // update meta header
            MetaHeader* hdrPtr = static_cast<MetaHeader*>(zmq_msg_data(&fMessage));
            hdrPtr->fSize = fSize;
            return true;
        }
        catch (bipc::interprocess_exception& e)
        {
            LOG(INFO) << "FairMQMessageSHM::SetUsedSize could not set used size: " << e.what();
            return false;
        }
    }
    else
    {
        LOG(ERROR) << "FairMQMessageSHM::SetUsedSize: cannot set used size higher than original.";
        return false;
    }
}

FairMQ::Transport FairMQMessageSHM::GetType() const
{
    return fTransportType;
}

void FairMQMessageSHM::Copy(const FairMQMessage& msg)
{
    if (fHandle < 0)
    {
        bipc::managed_shared_memory::handle_t otherHandle = static_cast<const FairMQMessageSHM&>(msg).fHandle;
        if (otherHandle)
        {
            if (InitializeChunk(msg.GetSize()))
            {
                memcpy(GetData(), msg.GetData(), msg.GetSize());
            }
        }
        else
        {
            LOG(ERROR) << "FairMQMessageSHM::Copy() fail: source message not initialized!";
        }
    }
    else
    {
        LOG(ERROR) << "FairMQMessageSHM::Copy() fail: target message already initialized!";
    }
}

void FairMQMessageSHM::Copy(const FairMQMessagePtr& msg)
{
    if (fHandle < 0)
    {
        bipc::managed_shared_memory::handle_t otherHandle = static_cast<FairMQMessageSHM*>(msg.get())->fHandle;
        if (otherHandle)
        {
            if (InitializeChunk(msg->GetSize()))
            {
                memcpy(GetData(), msg->GetData(), msg->GetSize());
            }
        }
        else
        {
            LOG(ERROR) << "FairMQMessageSHM::Copy() fail: source message not initialized!";
        }
    }
    else
    {
        LOG(ERROR) << "FairMQMessageSHM::Copy() fail: target message already initialized!";
    }
}

void FairMQMessageSHM::CloseMessage()
{
    if (fHandle >= 0 && !fQueued)
    {
        if (fRegionId == 0)
        {
            fManager.Segment().deallocate(fManager.Segment().get_address_from_handle(fHandle));
            fHandle = 0;
        }
        else
        {
            // send notification back to the receiver
            // RegionBlock block(fHandle, fSize);
            // if (fManager.GetRegionQueue(fRegionId).try_send(static_cast<void*>(&block), sizeof(RegionBlock), 0))
            // {
            //     // LOG(INFO) << "true";
            // }
            // // else
            // // {
            // //     LOG(DEBUG) << "could not send ack";
            // // }

            // timed version
            RegionBlock block(fHandle, fSize, fHint);
            bool success = false;
            do
            {
                auto sndTill = bpt::microsec_clock::universal_time() + bpt::milliseconds(200);
                if (!fRegionPtr)
                {
                    fRegionPtr = fManager.GetRemoteRegion(fRegionId);
                }
                if (fRegionPtr)
                {
                    if (fRegionPtr->fQueue->timed_send(&block, sizeof(RegionBlock), 0, sndTill))
                    {
                        success = true;
                    }
                    else
                    {
                        if (fInterrupted)
                        {
                            break;
                        }
                        LOG(DEBUG) << "region ack queue is full, retrying...";
                    }
                }
                else
                {
                    // LOG(WARN) << "region ack queue for id " << fRegionId << " no longer exist. Not sending ack";
                    success = true;
                }
            }
            while (!success);
        }
    }

    if (fMetaCreated)
    {
        if (zmq_msg_close(&fMessage) != 0)
        {
            LOG(ERROR) << "failed closing message, reason: " << zmq_strerror(errno);
        }
    }
}

FairMQMessageSHM::~FairMQMessageSHM()
{
    CloseMessage();
}

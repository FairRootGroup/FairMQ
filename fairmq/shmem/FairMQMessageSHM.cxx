/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include <string>
#include <cstdlib>

#include "FairMQMessageSHM.h"
#include "FairMQLogger.h"

using namespace std;
using namespace FairMQ::shmem;

static FairMQ::Transport gTransportType = FairMQ::Transport::SHM;

// uint64_t FairMQMessageSHM::fMessageID = 0;
// string FairMQMessageSHM::fDeviceID = string();
atomic<bool> FairMQMessageSHM::fInterrupted(false);

FairMQMessageSHM::FairMQMessageSHM()
    : fMessage()
    // , fOwner(nullptr)
    // , fReceiving(false)
    , fQueued(false)
    , fMetaCreated(false)
    , fHandle()
    , fChunkSize(0)
    , fLocalPtr(nullptr)
{
    if (zmq_msg_init(&fMessage) != 0)
    {
        LOG(ERROR) << "failed initializing message, reason: " << zmq_strerror(errno);
    }
    fMetaCreated = true;
}

// void FairMQMessageSHM::StringDeleter(void* /*data*/, void* str)
// {
//     delete static_cast<string*>(str);
// }

FairMQMessageSHM::FairMQMessageSHM(const size_t size)
    : fMessage()
    // , fOwner(nullptr)
    // , fReceiving(false)
    , fQueued(false)
    , fMetaCreated(false)
    , fHandle()
    , fChunkSize(0)
    , fLocalPtr(nullptr)
{
    InitializeChunk(size);
}

FairMQMessageSHM::FairMQMessageSHM(void* data, const size_t size, fairmq_free_fn* ffn, void* hint)
    : fMessage()
    // , fOwner(nullptr)
    // , fReceiving(false)
    , fQueued(false)
    , fMetaCreated(false)
    , fHandle()
    , fChunkSize(0)
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

bool FairMQMessageSHM::InitializeChunk(const size_t size)
{
    // string chunkID = fDeviceID + "c" + to_string(fMessageID);
    // string* ownerID = new string(fDeviceID + "o" + to_string(fMessageID));

    while (!fHandle)
    {
        try
        {
            fLocalPtr = Manager::Instance().Segment()->allocate(size);

            // fOwner = Manager::Instance().Segment()->construct<ShPtrOwner>(ownerID->c_str())(
            //     make_managed_shared_ptr(Manager::Instance().Segment()->construct<Chunk>(chunkID.c_str())(size),
            //                             *(Manager::Instance().Segment())));
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
        fHandle = Manager::Instance().Segment()->get_handle_from_address(fLocalPtr);
    }

    fChunkSize = size;

    if (zmq_msg_init_size(&fMessage, sizeof(MetaHeader)) != 0)
    {
        LOG(ERROR) << "failed initializing meta message, reason: " << zmq_strerror(errno);
        return false;
    }
    MetaHeader* metaPtr = new(zmq_msg_data(&fMessage)) MetaHeader();
    metaPtr->fSize = size;
    metaPtr->fHandle = fHandle;

    // if (zmq_msg_init_data(&fMessage, const_cast<char*>(ownerID->c_str()), ownerID->length(), StringDeleter, ownerID) != 0)
    // {
    //     LOG(ERROR) << "failed initializing meta message, reason: " << zmq_strerror(errno);
    // }
    fMetaCreated = true;

    // ++fMessageID;

    return true;
}

void FairMQMessageSHM::Rebuild()
{
    CloseMessage();

    // fReceiving = false;
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

    // fReceiving = false;
    fQueued = false;

    InitializeChunk(size);
}

void FairMQMessageSHM::Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint)
{
    CloseMessage();

    // fReceiving = false;
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

void* FairMQMessageSHM::GetMessage()
{
    return &fMessage;
}

void* FairMQMessageSHM::GetData()
{
    if (fLocalPtr)
    {
        return fLocalPtr;
    }
    else if (fHandle)
    {
        return Manager::Instance().Segment()->get_address_from_handle(fHandle);
    }
    else
    {
        // LOG(ERROR) << "Trying to get data of an empty shared memory message";
        return nullptr;
    }

    // if (fOwner)
    // {
    //     return fOwner->fPtr->GetData();
    // }
    // else
    // {
    //     LOG(ERROR) << "Trying to get data of an empty shared memory message";
    //     exit(EXIT_FAILURE);
    // }
}

size_t FairMQMessageSHM::GetSize()
{
    return fChunkSize;
    // if (fOwner)
    // {
    //     return fOwner->fPtr->GetSize();
    // }
    // else
    // {
    //     return 0;
    // }
}

void FairMQMessageSHM::SetMessage(void*, const size_t)
{
    // dummy method to comply with the interface. functionality not allowed in zeromq.
}

void FairMQMessageSHM::SetDeviceId(const string& /*deviceId*/)
{
    // fDeviceID = deviceId;
}

FairMQ::Transport FairMQMessageSHM::GetType() const
{
    return gTransportType;
}

void FairMQMessageSHM::Copy(const unique_ptr<FairMQMessage>& msg)
{
    if (!fHandle)
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

    // version with sharing the sent data
    // if (!fOwner)
    // {
    //     if (static_cast<FairMQMessageSHM*>(msg.get())->fOwner)
    //     {
    //         string* ownerID = new string(fDeviceID + "o" + to_string(fMessageID));

    //         bool success = false;

    //         do
    //         {
    //             try
    //             {
    //                 fOwner = Manager::Instance().Segment()->construct<ShPtrOwner>(ownerID->c_str())(*(static_cast<FairMQMessageSHM*>(msg.get())->fOwner));
    //                 success = true;
    //             }
    //             catch (bipc::bad_alloc& ba)
    //             {
    //                 LOG(WARN) << "Shared memory full...";
    //                 this_thread::sleep_for(chrono::milliseconds(10));
    //                 if (fInterrupted)
    //                 {
    //                     break;
    //                 }
    //                 else
    //                 {
    //                     continue;
    //                 }
    //             }
    //         }
    //         while (!success);

    //         if (zmq_msg_init_data(&fMessage, const_cast<char*>(ownerID->c_str()), ownerID->length(), StringDeleter, ownerID) != 0)
    //         {
    //             LOG(ERROR) << "failed initializing meta message, reason: " << zmq_strerror(errno);
    //         }

    //         ++fMessageID;
    //     }
    //     else
    //     {
    //         LOG(ERROR) << "FairMQMessageSHM::Copy() fail: source message not initialized!";
    //     }
    // }
    // else
    // {
    //     LOG(ERROR) << "FairMQMessageSHM::Copy() fail: target message already initialized!";
    // }
}

void FairMQMessageSHM::CloseMessage()
{
    // if (fReceiving)
    // {
    //     if (fOwner)
    //     {
    //         Manager::Instance().Segment()->destroy_ptr(fOwner);
    //         fOwner = nullptr;
    //     }
    //     else
    //     {
    //         LOG(ERROR) << "No shared pointer owner when closing a received message";
    //     }
    // }
    // else
    // {
        if (fHandle && !fQueued)
        {
            // LOG(WARN) << "Destroying unsent message";
            // Manager::Instance().Segment()->destroy_ptr(fHandle);
            Manager::Instance().Segment()->deallocate(Manager::Instance().Segment()->get_address_from_handle(fHandle));
            fHandle = 0;
        }
    // }

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

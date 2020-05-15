/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQMessageZMQ.cxx
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko, N. Winckler
 */


#include "FairMQMessageZMQ.h"
#include "FairMQLogger.h"
#include <fairmq/Tools.h>
#include "FairMQUnmanagedRegionZMQ.h"
#include <FairMQTransportFactory.h>

#include <cstring>

using namespace std;

FairMQMessageZMQ::FairMQMessageZMQ(FairMQTransportFactory* factory)
    : FairMQMessage{factory}
    , fUsedSizeModified(false)
    , fUsedSize()
    , fMsg(fair::mq::tools::make_unique<zmq_msg_t>())
    , fViewMsg(nullptr)
{
    if (zmq_msg_init(fMsg.get()) != 0)
    {
        LOG(error) << "failed initializing message, reason: " << zmq_strerror(errno);
    }
}

FairMQMessageZMQ::FairMQMessageZMQ(const size_t size, FairMQTransportFactory* factory)
    : FairMQMessage{factory}
    , fUsedSizeModified(false)
    , fUsedSize(size)
    , fMsg(fair::mq::tools::make_unique<zmq_msg_t>())
    , fViewMsg(nullptr)
{
    if (zmq_msg_init_size(fMsg.get(), size) != 0)
    {
        LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
    }
}

FairMQMessageZMQ::FairMQMessageZMQ(void* data, const size_t size, fairmq_free_fn* ffn, void* hint, FairMQTransportFactory* factory)
    : FairMQMessage{factory}
    , fUsedSizeModified(false)
    , fUsedSize()
    , fMsg(fair::mq::tools::make_unique<zmq_msg_t>())
    , fViewMsg(nullptr)
{
    if (zmq_msg_init_data(fMsg.get(), data, size, ffn, hint) != 0)
    {
        LOG(error) << "failed initializing message with data, reason: " << zmq_strerror(errno);
    }
}

FairMQMessageZMQ::FairMQMessageZMQ(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint, FairMQTransportFactory* factory)
    : FairMQMessage{factory}
    , fUsedSizeModified(false)
    , fUsedSize()
    , fMsg(fair::mq::tools::make_unique<zmq_msg_t>())
    , fViewMsg(nullptr)
{
    // FIXME: make this zero-copy:
    // simply taking over the provided buffer can casue premature delete, since region could be destroyed before the message is sent out.
    // Needs lifetime extension for the ZMQ region.
    if (zmq_msg_init_size(fMsg.get(), size) != 0)
    {
        LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
    }

    memcpy(zmq_msg_data(fMsg.get()), data, size);
    // call region callback
    auto ptr = static_cast<FairMQUnmanagedRegionZMQ*>(region.get());
    if (ptr->fBulkCallback) {
        ptr->fBulkCallback({{data, size, hint}});
    } else if (ptr->fCallback) {
        ptr->fCallback(data, size, hint);
    }

    // if (zmq_msg_init_data(fMsg.get(), data, size, [](void*, void*){}, nullptr) != 0)
    // {
    //     LOG(error) << "failed initializing message with data, reason: " << zmq_strerror(errno);
    // }
}

void FairMQMessageZMQ::Rebuild()
{
    CloseMessage();
    fMsg = fair::mq::tools::make_unique<zmq_msg_t>();
    if (zmq_msg_init(fMsg.get()) != 0)
    {
        LOG(error) << "failed initializing message, reason: " << zmq_strerror(errno);
    }
}

void FairMQMessageZMQ::Rebuild(const size_t size)
{
    CloseMessage();
    fMsg = fair::mq::tools::make_unique<zmq_msg_t>();
    if (zmq_msg_init_size(fMsg.get(), size) != 0)
    {
        LOG(error) << "failed initializing message with size, reason: " << zmq_strerror(errno);
    }
}

void FairMQMessageZMQ::Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint)
{
    CloseMessage();
    fMsg = fair::mq::tools::make_unique<zmq_msg_t>();
    if (zmq_msg_init_data(fMsg.get(), data, size, ffn, hint) != 0)
    {
        LOG(error) << "failed initializing message with data, reason: " << zmq_strerror(errno);
    }
}

zmq_msg_t* FairMQMessageZMQ::GetMessage() const
{
    if (!fViewMsg)
    {
        return fMsg.get();
    }
    else
    {
        return fViewMsg.get();
    }
}

void* FairMQMessageZMQ::GetData() const
{
    if (!fViewMsg)
    {
        return zmq_msg_data(fMsg.get());
    }
    else
    {
        return zmq_msg_data(fViewMsg.get());
    }
}

size_t FairMQMessageZMQ::GetSize() const
{
    if (fUsedSizeModified)
    {
        return fUsedSize;
    }
    else
    {
        return zmq_msg_size(fMsg.get());
    }
}

// To emulate shrinking, a new message is created with the new size (ViewMsg), that points to the original buffer with the new size.
// Once the "view message" is transfered, the original is destroyed.
// Used size is applied only once in ApplyUsedSize, which is called by the socket before sending.
// This function just updates the desired size until the actual "resizing" happens.
bool FairMQMessageZMQ::SetUsedSize(const size_t size)
{
    if (size <= zmq_msg_size(fMsg.get()))
    {
        fUsedSize = size;
        fUsedSizeModified = true;
        return true;
    }
    else
    {
        LOG(error) << "cannot set used size higher than original.";
        return false;
    }
}

void FairMQMessageZMQ::ApplyUsedSize()
{
    // Apply only once (before actual send).
    // The check is needed because a send could fail and can be reattempted by the user, in this case we do not want to modify buffer again.
    if (fUsedSizeModified && !fViewMsg)
    {
        fViewMsg = fair::mq::tools::make_unique<zmq_msg_t>();
        void* ptr = zmq_msg_data(fMsg.get());
        if (zmq_msg_init_data(fViewMsg.get(),
                                ptr,
                                fUsedSize,
                                [](void* /* data */, void* obj)
                                {
                                    zmq_msg_close(static_cast<zmq_msg_t*>(obj));
                                    delete static_cast<zmq_msg_t*>(obj);
                                },
                                fMsg.release()) != 0)
        {
            LOG(error) << "failed initializing view message, reason: " << zmq_strerror(errno);
        }
    }
}

void FairMQMessageZMQ::Copy(const FairMQMessage& msg)
{
    const FairMQMessageZMQ& zMsg = static_cast<const FairMQMessageZMQ&>(msg);
    // Shares the message buffer between msg and this fMsg.
    if (zmq_msg_copy(fMsg.get(), zMsg.GetMessage()) != 0)
    {
        LOG(error) << "failed copying message, reason: " << zmq_strerror(errno);
        return;
    }

    // if the target message has been resized, apply same to this message also
    if (zMsg.fUsedSizeModified)
    {
        fUsedSizeModified = true;
        fUsedSize = zMsg.fUsedSize;
    }
}

void FairMQMessageZMQ::CloseMessage()
{
    if (!fViewMsg)
    {
        if (zmq_msg_close(fMsg.get()) != 0)
        {
            LOG(error) << "failed closing message, reason: " << zmq_strerror(errno);
        }
        // reset the message object to allow reuse in Rebuild
        fMsg.reset(nullptr);
    }
    else
    {
        if (zmq_msg_close(fViewMsg.get()) != 0)
        {
            LOG(error) << "failed closing message, reason: " << zmq_strerror(errno);
        }
        // reset the message object to allow reuse in Rebuild
        fViewMsg.reset(nullptr);
    }
    fUsedSizeModified = false;
    fUsedSize = 0;
}

FairMQMessageZMQ::~FairMQMessageZMQ()
{
    CloseMessage();
}

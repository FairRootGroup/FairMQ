/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQMessageZMQ.cxx
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko, N. Winckler
 */

#include <cstring>
#include <cstdlib>

#include "FairMQMessageZMQ.h"
#include "FairMQLogger.h"

using namespace std;

FairMQMessageZMQ::FairMQMessageZMQ()
    : fMessage()
{
    if (zmq_msg_init(&fMessage) != 0)
    {
        LOG(ERROR) << "failed initializing message, reason: " << zmq_strerror(errno);
    }
}

FairMQMessageZMQ::FairMQMessageZMQ(const size_t size)
    : fMessage()
{
    if (zmq_msg_init_size(&fMessage, size) != 0)
    {
        LOG(ERROR) << "failed initializing message with size, reason: " << zmq_strerror(errno);
    }
}

FairMQMessageZMQ::FairMQMessageZMQ(void* data, const size_t size, fairmq_free_fn *ffn, void* hint)
    : fMessage()
{
    if (zmq_msg_init_data(&fMessage, data, size, ffn ? ffn : &CleanUp, hint) != 0)
    {
        LOG(ERROR) << "failed initializing message with data, reason: " << zmq_strerror(errno);
    }
}

void FairMQMessageZMQ::Rebuild()
{
    CloseMessage();
    if (zmq_msg_init(&fMessage) != 0)
    {
        LOG(ERROR) << "failed initializing message, reason: " << zmq_strerror(errno);
    }
}

void FairMQMessageZMQ::Rebuild(const size_t size)
{
    CloseMessage();
    if (zmq_msg_init_size(&fMessage, size) != 0)
    {
        LOG(ERROR) << "failed initializing message with size, reason: " << zmq_strerror(errno);
    }
}

void FairMQMessageZMQ::Rebuild(void* data, const size_t size, fairmq_free_fn *ffn, void* hint)
{
    CloseMessage();
    if (zmq_msg_init_data(&fMessage, data, size, ffn ? ffn : &CleanUp, hint) != 0)
    {
        LOG(ERROR) << "failed initializing message with data, reason: " << zmq_strerror(errno);
    }
}

void* FairMQMessageZMQ::GetMessage()
{
    return &fMessage;
}

void* FairMQMessageZMQ::GetData()
{
    return zmq_msg_data(&fMessage);
}

size_t FairMQMessageZMQ::GetSize()
{
    return zmq_msg_size(&fMessage);
}

void FairMQMessageZMQ::SetMessage(void*, const size_t)
{
    // dummy method to comply with the interface. functionality not allowed in zeromq.
}

void FairMQMessageZMQ::Copy(FairMQMessage* msg)
{
    // DEPRECATED: Use Copy(const unique_ptr<FairMQMessage>&)

    // Shares the message buffer between msg and this fMessage.
    if (zmq_msg_copy(&fMessage, (zmq_msg_t*)msg->GetMessage()) != 0)
    {
        LOG(ERROR) << "failed copying message, reason: " << zmq_strerror(errno);
    }

    // Alternatively, following code does a hard copy of the message, which allows to modify the original after making a copy, without affecting the new msg.

    // CloseMessage();
    // size_t size = msg->GetSize();
    // zmq_msg_init_size(&fMessage, size);
    // memcpy(zmq_msg_data(&fMessage), msg->GetData(), size);
}

void FairMQMessageZMQ::Copy(const unique_ptr<FairMQMessage>& msg)
{
    // Shares the message buffer between msg and this fMessage.
    if (zmq_msg_copy(&fMessage, (zmq_msg_t*)msg->GetMessage()) != 0)
    {
        LOG(ERROR) << "failed copying message, reason: " << zmq_strerror(errno);
    }

    // Alternatively, following code does a hard copy of the message, which allows to modify the original after making a copy, without affecting the new msg.

    // CloseMessage();
    // size_t size = msg->GetSize();
    // zmq_msg_init_size(&fMessage, size);
    // memcpy(zmq_msg_data(&fMessage), msg->GetData(), size);
}

inline void FairMQMessageZMQ::CloseMessage()
{
    if (zmq_msg_close(&fMessage) != 0)
    {
        LOG(ERROR) << "failed closing message, reason: " << zmq_strerror(errno);
    }
}

void FairMQMessageZMQ::CleanUp(void* data, void*)
{
    free(data);
}

FairMQMessageZMQ::~FairMQMessageZMQ()
{
    if (zmq_msg_close(&fMessage) != 0)
    {
        LOG(ERROR) << "failed closing message with data, reason: " << zmq_strerror(errno);
    }
}
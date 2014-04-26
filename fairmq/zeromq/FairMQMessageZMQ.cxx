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

FairMQMessageZMQ::FairMQMessageZMQ()
{
    int rc = zmq_msg_init(&fMessage);
    if (rc != 0)
    {
        LOG(ERROR) << "failed initializing message, reason: " << zmq_strerror(errno);
    }
}

FairMQMessageZMQ::FairMQMessageZMQ(size_t size)
{
    int rc = zmq_msg_init_size(&fMessage, size);
    if (rc != 0)
    {
        LOG(ERROR) << "failed initializing message with size, reason: " << zmq_strerror(errno);
    }
}

FairMQMessageZMQ::FairMQMessageZMQ(void* data, size_t size)
{
    int rc = zmq_msg_init_data(&fMessage, data, size, &CleanUp, NULL); // TODO: expose the cleanup function part in the interface?
    if (rc != 0)
    {
        LOG(ERROR) << "failed initializing message with data, reason: " << zmq_strerror(errno);
    }
}

void FairMQMessageZMQ::Rebuild()
{
    CloseMessage();
    int rc = zmq_msg_init(&fMessage);
    if (rc != 0)
    {
        LOG(ERROR) << "failed initializing message, reason: " << zmq_strerror(errno);
    }
}

void FairMQMessageZMQ::Rebuild(size_t size)
{
    CloseMessage();
    int rc = zmq_msg_init_size(&fMessage, size);
    if (rc != 0)
    {
        LOG(ERROR) << "failed initializing message with size, reason: " << zmq_strerror(errno);
    }
}

void FairMQMessageZMQ::Rebuild(void* data, size_t size)
{
    CloseMessage();
    int rc = zmq_msg_init_data(&fMessage, data, size, &CleanUp, NULL); // TODO: expose the cleanup function part in the interface?
    if (rc != 0)
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

void FairMQMessageZMQ::SetMessage(void* data, size_t size)
{
    // dummy method to comply with the interface. functionality not allowed in zeromq.
}

void FairMQMessageZMQ::Copy(FairMQMessage* msg)
{
    // Shares the message buffer between msg and this fMessage.
    int rc = zmq_msg_copy(&fMessage, (zmq_msg_t*)msg->GetMessage());
    if (rc != 0)
    {
        LOG(ERROR) << "failed copying message, reason: " << zmq_strerror(errno);
    }

    // Alternatively, following code does a hard copy of the message, which allows to modify the original after making a copy, without affecting the new msg.

    // CloseMessage();
    // size_t size = msg->GetSize();
    // zmq_msg_init_size(&fMessage, size);
    // std::memcpy(zmq_msg_data(&fMessage), msg->GetData(), size);
}

inline void FairMQMessageZMQ::CloseMessage()
{
    int rc = zmq_msg_close(&fMessage);
    if (rc != 0)
    {
        LOG(ERROR) << "failed closing message, reason: " << zmq_strerror(errno);
    }
}

void FairMQMessageZMQ::CleanUp(void* data, void* hint)
{
    free(data);
}

FairMQMessageZMQ::~FairMQMessageZMQ()
{
    int rc = zmq_msg_close(&fMessage);
    if (rc != 0)
    {
        LOG(ERROR) << "failed closing message with data, reason: " << zmq_strerror(errno);
    }
}
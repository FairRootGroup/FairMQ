/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQMessageNN.cxx
 *
 * @since 2013-12-05
 * @author A. Rybalchenko
 */

#include <cstring>
#include <stdlib.h>

#include <nanomsg/nn.h>

#include "FairMQMessageNN.h"
#include "FairMQLogger.h"

using namespace std;

FairMQMessageNN::FairMQMessageNN()
    : fMessage(NULL)
    , fSize(0)
    , fReceiving(false)
{
    fMessage = nn_allocmsg(0, 0);
    if (!fMessage)
    {
        LOG(ERROR) << "failed allocating message, reason: " << nn_strerror(errno);
    }
}

FairMQMessageNN::FairMQMessageNN(size_t size)
    : fMessage(NULL)
    , fSize(0)
    , fReceiving(false)
{
    fMessage = nn_allocmsg(size, 0);
    if (!fMessage)
    {
        LOG(ERROR) << "failed allocating message, reason: " << nn_strerror(errno);
    }
    fSize = size;
}


/* nanomsg does not offer support for creating a message out of an existing buffer,
 * therefore the following method is using memcpy. For more efficient handling,
 * create FairMQMessage object only with size parameter and fill it with data.
 * possible TODO: make this zero copy (will should then be as efficient as ZeroMQ).
*/
FairMQMessageNN::FairMQMessageNN(void* data, size_t size, fairmq_free_fn *ffn, void* hint)
    : fMessage(NULL)
    , fSize(0)
    , fReceiving(false)
{
    fMessage = nn_allocmsg(size, 0);
    if (!fMessage)
    {
        LOG(ERROR) << "failed allocating message, reason: " << nn_strerror(errno);
    }
    memcpy(fMessage, data, size);
    fSize = size;

    if (ffn)
    {
        ffn(data, hint);
    }
    else
    {
        if(data) free(data);
    }
}

void FairMQMessageNN::Rebuild()
{
    Clear();
    fReceiving = false;
}

void FairMQMessageNN::Rebuild(size_t size)
{
    Clear();
    fMessage = nn_allocmsg(size, 0);
    if (!fMessage)
    {
        LOG(ERROR) << "failed allocating message, reason: " << nn_strerror(errno);
    }
    fSize = size;
    fReceiving = false;
}

void FairMQMessageNN::Rebuild(void* data, size_t size, fairmq_free_fn *ffn, void* hint)
{
    Clear();
    fMessage = nn_allocmsg(size, 0);
    if (!fMessage)
    {
        LOG(ERROR) << "failed allocating message, reason: " << nn_strerror(errno);
    }
    memcpy(fMessage, data, size);
    fSize = size;
    fReceiving = false;

    if(ffn)
    {
        ffn(data, hint);
    }
    else
    {
        if(data) free(data);
    }
}

void* FairMQMessageNN::GetMessage()
{
    return fMessage;
}

void* FairMQMessageNN::GetData()
{
    return fMessage;
}

size_t FairMQMessageNN::GetSize()
{
    return fSize;
}

void FairMQMessageNN::SetMessage(void* data, size_t size)
{
    fMessage = data;
    fSize = size;
}

void FairMQMessageNN::Copy(FairMQMessage* msg)
{
    // DEPRECATED: Use Copy(const unique_ptr<FairMQMessage>&)

    if (fMessage)
    {
        if (nn_freemsg(fMessage) < 0)
        {
            LOG(ERROR) << "failed freeing message, reason: " << nn_strerror(errno);
        }
    }

    size_t size = msg->GetSize();

    fMessage = nn_allocmsg(size, 0);
    if (!fMessage)
    {
        LOG(ERROR) << "failed allocating message, reason: " << nn_strerror(errno);
    }
    memcpy(fMessage, msg->GetMessage(), size);
    fSize = size;
}

void FairMQMessageNN::Copy(const unique_ptr<FairMQMessage>& msg)
{
    if (fMessage)
    {
        if (nn_freemsg(fMessage) < 0)
        {
            LOG(ERROR) << "failed freeing message, reason: " << nn_strerror(errno);
        }
    }

    size_t size = msg->GetSize();

    fMessage = nn_allocmsg(size, 0);
    if (!fMessage)
    {
        LOG(ERROR) << "failed allocating message, reason: " << nn_strerror(errno);
    }
    memcpy(fMessage, msg->GetMessage(), size);
    fSize = size;
}

inline void FairMQMessageNN::Clear()
{
    if (nn_freemsg(fMessage) < 0)
    {
        LOG(ERROR) << "failed freeing message, reason: " << nn_strerror(errno);
    }
    else
    {
        fMessage = NULL;
        fSize = 0;
    }
}

FairMQMessageNN::~FairMQMessageNN()
{
    if (fReceiving)
    {
        int rc = nn_freemsg(fMessage);
        if (rc < 0)
        {
            LOG(ERROR) << "failed freeing message, reason: " << nn_strerror(errno);
        }
        else
        {
            fMessage = NULL;
            fSize = 0;
        }
    }
}

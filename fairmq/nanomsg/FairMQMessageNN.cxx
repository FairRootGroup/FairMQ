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

#include <nanomsg/nn.h>

#include "FairMQMessageNN.h"
#include "FairMQLogger.h"

FairMQMessageNN::FairMQMessageNN()
    : fSize(0)
    , fMessage(NULL)
    , fReceiving(false)
{
}

FairMQMessageNN::FairMQMessageNN(size_t size)
{
    fMessage = nn_allocmsg(size, 0);
    if (!fMessage)
    {
        LOG(ERROR) << "failed allocating message, reason: " << nn_strerror(errno);
    }
    fSize = size;
    fReceiving = false;
}


/* nanomsg does not offer support for creating a message out of an existing buffer,
 * therefore the following method is using memcpy. For more efficient handling,
 * create FairMQMessage object only with size parameter and fill it with data.
 * possible TODO: make this zero copy (will should then be as efficient as ZeroMQ).
*/
FairMQMessageNN::FairMQMessageNN(void* data, size_t size, fairmq_free_fn *ffn, void* hint)
{
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

void FairMQMessageNN::Rebuild()
{
    Clear();
    fSize = 0;
    fMessage = NULL;
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
    if (fMessage)
    {
        int rc = nn_freemsg(fMessage);
        if (rc < 0)
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
    std::memcpy(fMessage, msg->GetMessage(), size);
    fSize = size;
}

inline void FairMQMessageNN::Clear()
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

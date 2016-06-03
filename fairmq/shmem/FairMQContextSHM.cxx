/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include <zmq.h>

#include <boost/interprocess/managed_shared_memory.hpp>

#include "FairMQLogger.h"
#include "FairMQContextSHM.h"
#include "FairMQShmManager.h"

using namespace FairMQ::shmem;

FairMQContextSHM::FairMQContextSHM(int numIoThreads)
    : fContext()
{
    fContext = zmq_ctx_new();
    if (fContext == NULL)
    {
        LOG(ERROR) << "failed creating context, reason: " << zmq_strerror(errno);
        exit(EXIT_FAILURE);
    }

    if (zmq_ctx_set(fContext, ZMQ_IO_THREADS, numIoThreads) != 0)
    {
        LOG(ERROR) << "failed configuring context, reason: " << zmq_strerror(errno);
    }

    // Set the maximum number of allowed sockets on the context.
    if (zmq_ctx_set(fContext, ZMQ_MAX_SOCKETS, 10000) != 0)
    {
        LOG(ERROR) << "failed configuring context, reason: " << zmq_strerror(errno);
    }

    Manager::Instance().InitializeSegment("open_or_create", "FairMQSharedMemory", 2000000000);
    LOG(INFO) << "Created/Opened shared memory segment of 2,000,000,000 bytes. Available are " << Manager::Instance().Segment()->get_free_memory() << " bytes.";
}

FairMQContextSHM::~FairMQContextSHM()
{
    Close();

    if (boost::interprocess::shared_memory_object::remove("FairMQSharedMemory"))
    {
        LOG(INFO) << "Successfully removed shared memory after the device has stopped.";
    }
    else
    {
        LOG(INFO) << "Did not remove shared memory after the device stopped. Still in use?";
    }
}

void* FairMQContextSHM::GetContext()
{
    return fContext;
}

void FairMQContextSHM::Close()
{
    if (fContext == NULL)
    {
        return;
    }

    if (zmq_ctx_destroy(fContext) != 0)
    {
        if (errno == EINTR)
        {
            LOG(ERROR) << " failed closing context, reason: " << zmq_strerror(errno);
        }
        else
        {
            fContext = NULL;
            return;
        }
    }
}

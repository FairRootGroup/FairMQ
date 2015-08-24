/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQContextZMQ.cxx
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#include <cstdlib> // quick_exit()
#include <sstream>

#include "FairMQLogger.h"
#include "FairMQContextZMQ.h"

FairMQContextZMQ::FairMQContextZMQ(int numIoThreads)
    : fContext()
{
    fContext = zmq_ctx_new();
    if (fContext == NULL)
    {
        LOG(ERROR) << "failed creating context, reason: " << zmq_strerror(errno);
        quick_exit(EXIT_FAILURE);
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
}

FairMQContextZMQ::~FairMQContextZMQ()
{
    Close();
}

void* FairMQContextZMQ::GetContext()
{
    return fContext;
}

void FairMQContextZMQ::Close()
{
    if (fContext == NULL)
    {
        return;
    }

    if (zmq_ctx_destroy(fContext) != 0)
    {
        if (errno == EINTR) {
            LOG(ERROR) << " failed closing context, reason: " << zmq_strerror(errno);
        } else {
            fContext = NULL;
            return;
        }
    }
}

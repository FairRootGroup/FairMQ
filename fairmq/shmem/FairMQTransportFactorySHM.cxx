/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "zmq.h"
#include <boost/version.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include "FairMQLogger.h"
#include "FairMQShmManager.h"
#include "FairMQTransportFactorySHM.h"
#include "../options/FairMQProgOptions.h"

using namespace std;
using namespace FairMQ::shmem;

FairMQ::Transport FairMQTransportFactorySHM::fTransportType = FairMQ::Transport::SHM;

FairMQTransportFactorySHM::FairMQTransportFactorySHM()
    : fContext(nullptr)
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    LOG(DEBUG) << "Transport: Using ZeroMQ (" << major << "." << minor << "." << patch << ") & "
               << "boost::interprocess (" << (BOOST_VERSION / 100000) << "." << (BOOST_VERSION / 100 % 1000) << "." << (BOOST_VERSION % 100) << ")";

    fContext = zmq_ctx_new();
    if (!fContext)
    {
        LOG(ERROR) << "failed creating context, reason: " << zmq_strerror(errno);
        exit(EXIT_FAILURE);
    }
}

void FairMQTransportFactorySHM::Initialize(const FairMQProgOptions* config)
{
    int numIoThreads = 1;
    if (config)
    {
        numIoThreads = config->GetValue<int>("io-threads");
    }
    else
    {
        LOG(WARN) << "shmem: FairMQProgOptions not available! Using defaults.";
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
    LOG(DEBUG) << "shmem: created/opened shared memory segment of 2000000000 bytes. Available are " << Manager::Instance().Segment()->get_free_memory() << " bytes.";
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage() const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM());
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage(const size_t size) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM(size));
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM(data, size, ffn, hint));
}

FairMQSocketPtr FairMQTransportFactorySHM::CreateSocket(const string& type, const string& name, const string& id /*= ""*/) const
{
    assert(fContext);
    return unique_ptr<FairMQSocket>(new FairMQSocketSHM(type, name, id, fContext));
}

FairMQPollerPtr FairMQTransportFactorySHM::CreatePoller(const vector<FairMQChannel>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerSHM(channels));
}

FairMQPollerPtr FairMQTransportFactorySHM::CreatePoller(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerSHM(channelsMap, channelList));
}

FairMQPollerPtr FairMQTransportFactorySHM::CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerSHM(cmdSocket, dataSocket));
}

void FairMQTransportFactorySHM::Shutdown()
{
    if (zmq_ctx_shutdown(fContext) != 0)
    {
        LOG(ERROR) << "shmem: failed shutting down context, reason: " << zmq_strerror(errno);
    }
}

void FairMQTransportFactorySHM::Terminate()
{
    if (fContext)
    {
        if (zmq_ctx_term(fContext) != 0)
        {
            if (errno == EINTR)
            {
                LOG(ERROR) << "shmem: failed closing context, reason: " << zmq_strerror(errno);
            }
            else
            {
                fContext = NULL;
                return;
            }
        }
    }
    else
    {
        LOG(ERROR) << "shmem: Terminate(): context now available for shutdown";
    }

    if (boost::interprocess::shared_memory_object::remove("FairMQSharedMemory"))
    {
        LOG(DEBUG) << "shmem: successfully removed shared memory segment after the device has stopped.";
    }
    else
    {
        LOG(DEBUG) << "shmem: did not remove shared memory segment after the device stopped. Already removed?";
    }
}

FairMQ::Transport FairMQTransportFactorySHM::GetType() const
{
    return fTransportType;
}


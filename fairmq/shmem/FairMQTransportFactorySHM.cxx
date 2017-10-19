/********************************************************************************
 * Copyright (C) 2016-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQLogger.h"
#include "FairMQShmManager.h"
#include "FairMQTransportFactorySHM.h"

#include <zmq.h>

#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/interprocess/sync/scoped_lock.hpp>

#include <chrono>
#include <thread>
#include <cstdlib> // std::system

using namespace std;
using namespace fair::mq::shmem;
namespace bipc = boost::interprocess;
namespace bfs = boost::filesystem;
namespace bpt = boost::posix_time;

FairMQ::Transport FairMQTransportFactorySHM::fTransportType = FairMQ::Transport::SHM;

FairMQTransportFactorySHM::FairMQTransportFactorySHM(const string& id, const FairMQProgOptions* config)
    : FairMQTransportFactory(id)
    , fContext(nullptr)
    , fHeartbeatSocket(nullptr)
    , fHeartbeatThread()
    , fSendHeartbeats(true)
    , fShMutex(bipc::open_or_create, "fairmq_shmem_mutex")
    , fDeviceCounter(nullptr)
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
    
    int numIoThreads = 1;
    size_t segmentSize = 2000000000;
    string segmentName = "fairmq_shmem_main";
    if (config)
    {
        numIoThreads = config->GetValue<int>("io-threads");
        segmentSize = config->GetValue<size_t>("shm-segment-size");
        segmentName = config->GetValue<string>("shm-segment-name");
    }
    else
    {
        LOG(WARN) << "shmem: FairMQProgOptions not available! Using defaults.";
    }

    if (zmq_ctx_set(fContext, ZMQ_IO_THREADS, numIoThreads) != 0)
    {
        LOG(ERROR) << "shmem: failed configuring context, reason: " << zmq_strerror(errno);
    }

    // Set the maximum number of allowed sockets on the context.
    if (zmq_ctx_set(fContext, ZMQ_MAX_SOCKETS, 10000) != 0)
    {
        LOG(ERROR) << "shmem: failed configuring context, reason: " << zmq_strerror(errno);
    }

    Manager::Instance().InitializeSegment("open_or_create", segmentName, segmentSize);
    LOG(DEBUG) << "shmem: created/opened shared memory segment of " << segmentSize << " bytes. Available are " << Manager::Instance().Segment()->get_free_memory() << " bytes.";

    {
        bipc::scoped_lock<bipc::named_mutex> lock(fShMutex);

        fDeviceCounter = Manager::Instance().Segment()->find<DeviceCounter>(bipc::unique_instance).first;
        if (fDeviceCounter)
        {
            LOG(DEBUG) << "shmem: device counter found, with value of " << fDeviceCounter->fCount << ". incrementing.";
            (fDeviceCounter->fCount)++;
            LOG(DEBUG) << "shmem: incremented device counter, now: " << fDeviceCounter->fCount;
        }
        else
        {
            LOG(DEBUG) << "shmem: no device counter found, creating one and initializing with 1";
            fDeviceCounter = Manager::Instance().Segment()->construct<DeviceCounter>(bipc::unique_instance)(1);
            LOG(DEBUG) << "shmem: initialized device counter with: " << fDeviceCounter->fCount;
        }

        // start shm monitor
        // try
        // {
        //     MonitorStatus* monitorStatus = fManagementSegment.find<MonitorStatus>(bipc::unique_instance).first;
        //     if (monitorStatus == nullptr)
        //     {
        //         LOG(DEBUG) << "shmem: no shmmonitor found, starting...";
        //         StartMonitor();
        //     }
        //     else
        //     {
        //         LOG(DEBUG) << "shmem: found shmmonitor in fairmq_shmem_management.";
        //     }
        // }
        // catch (std::exception& e)
        // {
        //     LOG(ERROR) << "shmem: Exception during shmmonitor initialization: " << e.what() << ", application will now exit";
        //     exit(EXIT_FAILURE);
        // }
    }

    fSendHeartbeats = true;
    fHeartbeatThread = thread(&FairMQTransportFactorySHM::SendHeartbeats, this);
}

void FairMQTransportFactorySHM::StartMonitor()
{
    int numTries = 0;

    if (!bfs::exists(bfs::path("shmmonitor")))
    {
        LOG(ERROR) << "Could not find shmmonitor. Is it in the PATH? Monitor not started";
        return;
    }

    // TODO: replace with Boost.Process once boost 1.64 is available
    int r = system("shmmonitor --self-destruct &");
    LOG(DEBUG) << r;

    do
    {
        MonitorStatus* monitorStatus = Manager::Instance().ManagementSegment().find<MonitorStatus>(bipc::unique_instance).first;
        if (monitorStatus)
        {
            LOG(DEBUG) << "shmem: shmmonitor started";
            break;
        }
        else
        {
            this_thread::sleep_for(std::chrono::milliseconds(10));
            if (++numTries > 100)
            {
                LOG(ERROR) << "Did not get response from shmmonitor after " << 10 * 100 << " milliseconds. Exiting.";
                exit(EXIT_FAILURE);
            }
        }
    }
    while (true);
}

void FairMQTransportFactorySHM::SendHeartbeats()
{
    while (fSendHeartbeats)
    {
        try
        {
            bipc::message_queue mq(bipc::open_only, "fairmq_shmem_control_queue");
            bool heartbeat = true;
            bpt::ptime sndTill = bpt::microsec_clock::universal_time() + bpt::milliseconds(100);
            if (mq.timed_send(&heartbeat, sizeof(heartbeat), 0, sndTill))
            {
                this_thread::sleep_for(chrono::milliseconds(100));
            }
            else
            {
                LOG(DEBUG) << "control queue timeout";
            }
        }
        catch (bipc::interprocess_exception& ie)
        {
            this_thread::sleep_for(chrono::milliseconds(500));
            // LOG(WARN) << "no fairmq_shmem_control_queue found";
        }
    }
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

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage(FairMQUnmanagedRegionPtr& region, void* data, const size_t size) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM(region, data, size));
}

FairMQSocketPtr FairMQTransportFactorySHM::CreateSocket(const string& type, const string& name) const
{
    assert(fContext);
    return unique_ptr<FairMQSocket>(new FairMQSocketSHM(type, name, GetId(), fContext));
}

FairMQPollerPtr FairMQTransportFactorySHM::CreatePoller(const vector<FairMQChannel>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerSHM(channels));
}

FairMQPollerPtr FairMQTransportFactorySHM::CreatePoller(const std::vector<const FairMQChannel*>& channels) const
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

FairMQUnmanagedRegionPtr FairMQTransportFactorySHM::CreateUnmanagedRegion(const size_t size) const
{
    return unique_ptr<FairMQUnmanagedRegion>(new FairMQUnmanagedRegionSHM(size));
}

FairMQTransportFactorySHM::~FairMQTransportFactorySHM()
{
    fSendHeartbeats = false;
    fHeartbeatThread.join();

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
                fContext = nullptr;
                return;
            }
        }
    }
    else
    {
        LOG(ERROR) << "shmem: Terminate(): context not available for shutdown";
    }

    { // mutex scope
        bipc::scoped_lock<bipc::named_mutex> lock(fShMutex);

        (fDeviceCounter->fCount)--;

        if (fDeviceCounter->fCount == 0)
        {
            LOG(DEBUG) << "shmem: last 'fairmq_shmem_main' user, removing segment.";

            Manager::Instance().Remove();
        }
        else
        {
            LOG(DEBUG) << "shmem: other 'fairmq_shmem_main' users present (" << fDeviceCounter->fCount << "), not removing it.";
        }
    }
}

FairMQ::Transport FairMQTransportFactorySHM::GetType() const
{
    return fTransportType;
}

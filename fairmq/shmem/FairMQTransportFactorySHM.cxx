/********************************************************************************
 * Copyright (C) 2016-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQLogger.h"
#include "FairMQTransportFactorySHM.h"

#include <zmq.h>

#include <boost/version.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/interprocess/sync/scoped_lock.hpp>

#include <chrono>
#include <thread>
#include <cstdlib> // getenv

using namespace std;
using namespace fair::mq::shmem;

namespace bfs = boost::filesystem;
namespace bpt = boost::posix_time;
namespace bipc = boost::interprocess;

fair::mq::Transport FairMQTransportFactorySHM::fTransportType = fair::mq::Transport::SHM;

FairMQTransportFactorySHM::FairMQTransportFactorySHM(const string& id, const FairMQProgOptions* config)
    : FairMQTransportFactory(id)
    , fDeviceId(id)
    , fSessionName("default")
    , fContext(nullptr)
    , fHeartbeatThread()
    , fSendHeartbeats(true)
    , fShMutex(nullptr)
    , fDeviceCounter(nullptr)
    , fManager(nullptr)
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    LOG(debug) << "Transport: Using ZeroMQ (" << major << "." << minor << "." << patch << ") & "
               << "boost::interprocess (" << (BOOST_VERSION / 100000) << "." << (BOOST_VERSION / 100 % 1000) << "." << (BOOST_VERSION % 100) << ")";

    fContext = zmq_ctx_new();
    if (!fContext)
    {
        LOG(error) << "failed creating context, reason: " << zmq_strerror(errno);
        exit(EXIT_FAILURE);
    }

    int numIoThreads = 1;
    size_t segmentSize = 2000000000;
    bool autolaunchMonitor = false;
    if (config)
    {
        numIoThreads = config->GetValue<int>("io-threads");
        fSessionName = config->GetValue<string>("session");
        segmentSize = config->GetValue<size_t>("shm-segment-size");
        autolaunchMonitor = config->GetValue<bool>("shm-monitor");
    }
    else
    {
        LOG(warn) << "FairMQProgOptions not available! Using defaults.";
    }

    fSessionName.resize(8, '_'); // shorten the session name, to accommodate for name size limit on some systems (MacOS)

    try
    {
        fShMutex = fair::mq::tools::make_unique<bipc::named_mutex>(bipc::open_or_create, string("fmq_" + fSessionName + "_mtx").c_str());

        if (zmq_ctx_set(fContext, ZMQ_IO_THREADS, numIoThreads) != 0)
        {
            LOG(error) << "failed configuring context, reason: " << zmq_strerror(errno);
        }

        // Set the maximum number of allowed sockets on the context.
        if (zmq_ctx_set(fContext, ZMQ_MAX_SOCKETS, 10000) != 0)
        {
            LOG(error) << "failed configuring context, reason: " << zmq_strerror(errno);
        }

        fManager = fair::mq::tools::make_unique<Manager>(fSessionName, segmentSize);
        LOG(debug) << "created/opened shared memory segment '" << "fmq_" << fSessionName << "_main" << "' of " << segmentSize << " bytes. Available are " << fManager->Segment().get_free_memory() << " bytes.";

        {
            bipc::scoped_lock<bipc::named_mutex> lock(*fShMutex);

            fDeviceCounter = fManager->Segment().find<DeviceCounter>(bipc::unique_instance).first;
            if (fDeviceCounter)
            {
                LOG(debug) << "device counter found, with value of " << fDeviceCounter->fCount << ". incrementing.";
                (fDeviceCounter->fCount)++;
                LOG(debug) << "incremented device counter, now: " << fDeviceCounter->fCount;
            }
            else
            {
                LOG(debug) << "no device counter found, creating one and initializing with 1";
                fDeviceCounter = fManager->Segment().construct<DeviceCounter>(bipc::unique_instance)(1);
                LOG(debug) << "initialized device counter with: " << fDeviceCounter->fCount;
            }

            // start shm monitor
            if (autolaunchMonitor)
            {
                try
                {
                    MonitorStatus* monitorStatus = fManager->ManagementSegment().find<MonitorStatus>(bipc::unique_instance).first;
                    if (monitorStatus == nullptr)
                    {
                        LOG(debug) << "no fairmq-shmmonitor found, starting...";
                        StartMonitor();
                    }
                    else
                    {
                        LOG(debug) << "found fairmq-shmmonitor.";
                    }
                }
                catch (exception& e)
                {
                    LOG(error) << "Exception during fairmq-shmmonitor initialization: " << e.what() << ", application will now exit";
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    catch(bipc::interprocess_exception& e)
    {
        LOG(error) << "Could not initialize shared memory transport: " << e.what();
        throw runtime_error("Cannot update configuration. Socket method (bind/connect) not specified.");
    }

    fSendHeartbeats = true;
    fHeartbeatThread = thread(&FairMQTransportFactorySHM::SendHeartbeats, this);
}

void FairMQTransportFactorySHM::StartMonitor()
{
    int numTries = 0;

    auto env = boost::this_process::environment();

    vector<bfs::path> ownPath = boost::this_process::path();

    if (const char* fmqp = getenv("FAIRMQ_PATH"))
    {
        ownPath.insert(ownPath.begin(), bfs::path(fmqp));
    }

    bfs::path p = boost::process::search_path("fairmq-shmmonitor", ownPath);

    if (!p.empty())
    {
        boost::process::spawn(p, "-x", "-s", fSessionName, "-d", "-t", "2000", env);

        do
        {
            MonitorStatus* monitorStatus = fManager->ManagementSegment().find<MonitorStatus>(bipc::unique_instance).first;
            if (monitorStatus)
            {
                LOG(debug) << "fairmq-shmmonitor started";
                break;
            }
            else
            {
                this_thread::sleep_for(chrono::milliseconds(10));
                if (++numTries > 1000)
                {
                    LOG(error) << "Did not get response from fairmq-shmmonitor after " << 10 * 1000 << " milliseconds. Exiting.";
                    exit(EXIT_FAILURE);
                }
            }
        }
        while (true);
    }
    else
    {
        LOG(warn) << "could not find fairmq-shmmonitor in the path";
    }
}

void FairMQTransportFactorySHM::SendHeartbeats()
{
    string controlQueueName("fmq_" + fSessionName + "_cq");
    while (fSendHeartbeats)
    {
        try
        {
            bipc::message_queue mq(bipc::open_only, controlQueueName.c_str());
            bpt::ptime sndTill = bpt::microsec_clock::universal_time() + bpt::milliseconds(100);
            if (mq.timed_send(fDeviceId.c_str(), fDeviceId.size(), 0, sndTill))
            {
                this_thread::sleep_for(chrono::milliseconds(100));
            }
            else
            {
                LOG(debug) << "control queue timeout";
            }
        }
        catch (bipc::interprocess_exception& ie)
        {
            this_thread::sleep_for(chrono::milliseconds(500));
            // LOG(warn) << "no " << controlQueueName << " found";
        }
    }
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage() const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM(*fManager));
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage(const size_t size) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM(*fManager, size));
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM(*fManager, data, size, ffn, hint));
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint) const
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM(*fManager, region, data, size, hint));
}

FairMQSocketPtr FairMQTransportFactorySHM::CreateSocket(const string& type, const string& name) const
{
    assert(fContext);
    return unique_ptr<FairMQSocket>(new FairMQSocketSHM(*fManager, type, name, GetId(), fContext));
}

FairMQPollerPtr FairMQTransportFactorySHM::CreatePoller(const vector<FairMQChannel>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerSHM(channels));
}

FairMQPollerPtr FairMQTransportFactorySHM::CreatePoller(const vector<const FairMQChannel*>& channels) const
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

FairMQUnmanagedRegionPtr FairMQTransportFactorySHM::CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback) const
{
    return unique_ptr<FairMQUnmanagedRegion>(new FairMQUnmanagedRegionSHM(*fManager, size, callback));
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
                LOG(error) << "failed closing context, reason: " << zmq_strerror(errno);
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
        LOG(error) << "context not available for shutdown";
    }

    bool lastRemoved = false;

    { // mutex scope
        bipc::scoped_lock<bipc::named_mutex> lock(*fShMutex);

        (fDeviceCounter->fCount)--;

        if (fDeviceCounter->fCount == 0)
        {
            LOG(debug) << "last segment user, removing segment.";

            fManager->RemoveSegment();
            lastRemoved = true;
        }
        else
        {
            LOG(debug) << "other segment users present (" << fDeviceCounter->fCount << "), not removing it.";
        }
    }

    if (lastRemoved)
    {
        boost::interprocess::named_mutex::remove(string("fmq_" + fSessionName + "_mtx").c_str());
    }
}

fair::mq::Transport FairMQTransportFactorySHM::GetType() const
{
    return fTransportType;
}

/********************************************************************************
 * Copyright (C) 2016-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQLogger.h"
#include "FairMQTransportFactorySHM.h"

#include <fairmq/Tools.h>

#include <zmq.h>

#include <boost/version.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/interprocess/sync/scoped_lock.hpp>

#include <chrono>
#include <thread>
#include <cstdlib> // getenv

using namespace std;
using namespace fair::mq::shmem;

namespace bpt = ::boost::posix_time;
namespace bipc = ::boost::interprocess;

fair::mq::Transport FairMQTransportFactorySHM::fTransportType = fair::mq::Transport::SHM;

FairMQTransportFactorySHM::FairMQTransportFactorySHM(const string& id, const FairMQProgOptions* config)
    : FairMQTransportFactory(id)
    , fDeviceId(id)
    , fShmId()
    , fZMQContext(nullptr)
    , fManager(nullptr)
    , fHeartbeatThread()
    , fSendHeartbeats(true)
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    LOG(debug) << "Transport: Using ZeroMQ (" << major << "." << minor << "." << patch << ") & "
               << "boost::interprocess (" << (BOOST_VERSION / 100000) << "." << (BOOST_VERSION / 100 % 1000) << "." << (BOOST_VERSION % 100) << ")";

    fZMQContext = zmq_ctx_new();
    if (!fZMQContext) {
        throw runtime_error(fair::mq::tools::ToString("failed creating context, reason: ", zmq_strerror(errno)));
    }

    int numIoThreads = 1;
    string sessionName = "default";
    size_t segmentSize = 2000000000;
    bool autolaunchMonitor = false;
    if (config) {
        numIoThreads = config->GetValue<int>("io-threads");
        sessionName = config->GetValue<string>("session");
        segmentSize = config->GetValue<size_t>("shm-segment-size");
        autolaunchMonitor = config->GetValue<bool>("shm-monitor");
    } else {
        LOG(debug) << "FairMQProgOptions not available! Using defaults.";
    }

    fShmId = buildShmIdFromSessionIdAndUserId(sessionName);

    try {
        if (zmq_ctx_set(fZMQContext, ZMQ_IO_THREADS, numIoThreads) != 0) {
            LOG(error) << "failed configuring context, reason: " << zmq_strerror(errno);
        }

        // Set the maximum number of allowed sockets on the context.
        if (zmq_ctx_set(fZMQContext, ZMQ_MAX_SOCKETS, 10000) != 0) {
            LOG(error) << "failed configuring context, reason: " << zmq_strerror(errno);
        }

        fManager = fair::mq::tools::make_unique<Manager>(fShmId, segmentSize);

        if (autolaunchMonitor) {
            fManager->StartMonitor();
        }
    } catch (bipc::interprocess_exception& e) {
        LOG(error) << "Could not initialize shared memory transport: " << e.what();
        throw runtime_error(fair::mq::tools::ToString("Could not initialize shared memory transport: ", e.what()));
    }

    fSendHeartbeats = true;
    fHeartbeatThread = thread(&FairMQTransportFactorySHM::SendHeartbeats, this);
}

void FairMQTransportFactorySHM::SendHeartbeats()
{
    string controlQueueName("fmq_" + fShmId + "_cq");
    while (fSendHeartbeats) {
        try {
            bipc::message_queue mq(bipc::open_only, controlQueueName.c_str());
            bpt::ptime sndTill = bpt::microsec_clock::universal_time() + bpt::milliseconds(100);
            if (mq.timed_send(fDeviceId.c_str(), fDeviceId.size(), 0, sndTill)) {
                this_thread::sleep_for(chrono::milliseconds(100));
            } else {
                LOG(debug) << "control queue timeout";
            }
        } catch (bipc::interprocess_exception& ie) {
            this_thread::sleep_for(chrono::milliseconds(500));
            // LOG(warn) << "no " << controlQueueName << " found";
        }
    }
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage()
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM(*fManager, this));
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage(const size_t size)
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM(*fManager, size, this));
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint)
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM(*fManager, data, size, ffn, hint, this));
}

FairMQMessagePtr FairMQTransportFactorySHM::CreateMessage(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint)
{
    return unique_ptr<FairMQMessage>(new FairMQMessageSHM(*fManager, region, data, size, hint, this));
}

FairMQSocketPtr FairMQTransportFactorySHM::CreateSocket(const string& type, const string& name)
{
    assert(fZMQContext);
    return unique_ptr<FairMQSocket>(new FairMQSocketSHM(*fManager, type, name, GetId(), fZMQContext, this));
}

FairMQPollerPtr FairMQTransportFactorySHM::CreatePoller(const vector<FairMQChannel>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerSHM(channels));
}

FairMQPollerPtr FairMQTransportFactorySHM::CreatePoller(const vector<FairMQChannel*>& channels) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerSHM(channels));
}

FairMQPollerPtr FairMQTransportFactorySHM::CreatePoller(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList) const
{
    return unique_ptr<FairMQPoller>(new FairMQPollerSHM(channelsMap, channelList));
}

FairMQUnmanagedRegionPtr FairMQTransportFactorySHM::CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback, const std::string& path /* = "" */, int flags /* = 0 */) const
{
    return unique_ptr<FairMQUnmanagedRegion>(new FairMQUnmanagedRegionSHM(*fManager, size, callback, path, flags));
}

fair::mq::Transport FairMQTransportFactorySHM::GetType() const
{
    return fTransportType;
}

FairMQTransportFactorySHM::~FairMQTransportFactorySHM()
{
    fSendHeartbeats = false;
    fHeartbeatThread.join();

    if (fZMQContext) {
        if (zmq_ctx_term(fZMQContext) != 0) {
            if (errno == EINTR) {
                LOG(error) << "failed closing context, reason: " << zmq_strerror(errno);
            } else {
                fZMQContext = nullptr;
                return;
            }
        }
    } else {
        LOG(error) << "context not available for shutdown";
    }
}

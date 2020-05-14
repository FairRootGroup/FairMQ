/********************************************************************************
 * Copyright (C) 2016-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "TransportFactory.h"

#include <FairMQLogger.h>
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

namespace bpt = ::boost::posix_time;
namespace bipc = ::boost::interprocess;

namespace fair
{
namespace mq
{
namespace shmem
{

Transport TransportFactory::fTransportType = Transport::SHM;

TransportFactory::TransportFactory(const string& id, const ProgOptions* config)
    : fair::mq::TransportFactory(id)
    , fDeviceId(id)
    , fShmId()
    , fZMQContext(nullptr)
    , fManager(nullptr)
    , fHeartbeatThread()
    , fSendHeartbeats(true)
    , fMsgCounter(0)
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    LOG(debug) << "Transport: Using ZeroMQ (" << major << "." << minor << "." << patch << ") & "
               << "boost::interprocess (" << (BOOST_VERSION / 100000) << "." << (BOOST_VERSION / 100 % 1000) << "." << (BOOST_VERSION % 100) << ")";

    fZMQContext = zmq_ctx_new();
    if (!fZMQContext) {
        throw runtime_error(tools::ToString("failed creating context, reason: ", zmq_strerror(errno)));
    }

    int numIoThreads = 1;
    string sessionName = "default";
    size_t segmentSize = 2000000000;
    bool autolaunchMonitor = false;
    if (config) {
        numIoThreads = config->GetProperty<int>("io-threads", numIoThreads);
        sessionName = config->GetProperty<string>("session", sessionName);
        segmentSize = config->GetProperty<size_t>("shm-segment-size", segmentSize);
        autolaunchMonitor = config->GetProperty<bool>("shm-monitor", autolaunchMonitor);
    } else {
        LOG(debug) << "ProgOptions not available! Using defaults.";
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

        if (autolaunchMonitor) {
            Manager::StartMonitor(fShmId);
        }

        fManager = tools::make_unique<Manager>(fShmId, segmentSize);

    } catch (bipc::interprocess_exception& e) {
        LOG(error) << "Could not initialize shared memory transport: " << e.what();
        throw runtime_error(tools::ToString("Could not initialize shared memory transport: ", e.what()));
    }

    fSendHeartbeats = true;
    fHeartbeatThread = thread(&TransportFactory::SendHeartbeats, this);
}

void TransportFactory::SendHeartbeats()
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

MessagePtr TransportFactory::CreateMessage()
{
    return tools::make_unique<Message>(*fManager, this);
}

MessagePtr TransportFactory::CreateMessage(const size_t size)
{
    return tools::make_unique<Message>(*fManager, size, this);
}

MessagePtr TransportFactory::CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint)
{
    return tools::make_unique<Message>(*fManager, data, size, ffn, hint, this);
}

MessagePtr TransportFactory::CreateMessage(UnmanagedRegionPtr& region, void* data, const size_t size, void* hint)
{
    return tools::make_unique<Message>(*fManager, region, data, size, hint, this);
}

SocketPtr TransportFactory::CreateSocket(const string& type, const string& name)
{
    assert(fZMQContext);
    return tools::make_unique<Socket>(*fManager, type, name, GetId(), fZMQContext, this);
}

PollerPtr TransportFactory::CreatePoller(const vector<FairMQChannel>& channels) const
{
    return tools::make_unique<Poller>(channels);
}

PollerPtr TransportFactory::CreatePoller(const vector<FairMQChannel*>& channels) const
{
    return tools::make_unique<Poller>(channels);
}

PollerPtr TransportFactory::CreatePoller(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList) const
{
    return tools::make_unique<Poller>(channelsMap, channelList);
}

UnmanagedRegionPtr TransportFactory::CreateUnmanagedRegion(const size_t size, RegionCallback callback, const std::string& path /* = "" */, int flags /* = 0 */)
{
    return tools::make_unique<UnmanagedRegion>(*fManager, size, 0, callback, nullptr, path, flags, this);
}

UnmanagedRegionPtr TransportFactory::CreateUnmanagedRegion(const size_t size, RegionBulkCallback bulkCallback, const std::string& path /* = "" */, int flags /* = 0 */)
{
    return tools::make_unique<UnmanagedRegion>(*fManager, size, 0, nullptr, bulkCallback, path, flags, this);
}

UnmanagedRegionPtr TransportFactory::CreateUnmanagedRegion(const size_t size, const int64_t userFlags, RegionCallback callback, const std::string& path /* = "" */, int flags /* = 0 */)
{
    return tools::make_unique<UnmanagedRegion>(*fManager, size, userFlags, callback, nullptr, path, flags, this);
}

UnmanagedRegionPtr TransportFactory::CreateUnmanagedRegion(const size_t size, const int64_t userFlags, RegionBulkCallback bulkCallback, const std::string& path /* = "" */, int flags /* = 0 */)
{
    return tools::make_unique<UnmanagedRegion>(*fManager, size, userFlags, nullptr, bulkCallback, path, flags, this);
}

void TransportFactory::SubscribeToRegionEvents(RegionEventCallback callback)
{
    fManager->SubscribeToRegionEvents(callback);
}

bool TransportFactory::SubscribedToRegionEvents()
{
    return fManager->SubscribedToRegionEvents();
}

void TransportFactory::UnsubscribeFromRegionEvents()
{
    fManager->UnsubscribeFromRegionEvents();
}

vector<fair::mq::RegionInfo> TransportFactory::GetRegionInfo()
{
    return fManager->GetRegionInfo();
}

Transport TransportFactory::GetType() const
{
    return fTransportType;
}

void TransportFactory::Reset()
{
    if (fMsgCounter.load() != 0) {
        LOG(error) << "Message counter during Reset expected to be 0, found: " << fMsgCounter.load();
        throw MessageError(tools::ToString("Message counter during Reset expected to be 0, found: ", fMsgCounter.load()));
    }
}


TransportFactory::~TransportFactory()
{
    LOG(debug) << "Destroying Shared Memory transport...";
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

} // namespace shmem
} // namespace mq
} // namespace fair

/********************************************************************************
 * Copyright (C) 2016-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "TransportFactory.h"
#include "Poller.h"
#include "Socket.h"
#include "UnmanagedRegionImpl.h"

#include <fairmq/ProgOptions.h>
#include <fairmq/tools/Strings.h>

#include <fairlogger/Logger.h>
#include <zmq.h>

#include <memory> // unique_ptr, make_unique
#include <stdexcept>

namespace fair::mq::shmem
{
TransportFactory::TransportFactory(const std::string& deviceId, const ProgOptions* config)
    : fair::mq::TransportFactory(deviceId)
    , fZmqCtx(zmq_ctx_new())
    , fManager(nullptr)
{
    int major = 0, minor = 0, patch = 0;
    zmq_version(&major, &minor, &patch);
    LOG(debug) << "Transport: Using ZeroMQ (" << major << "." << minor << "." << patch << ") & "
               << "boost::interprocess (" << (BOOST_VERSION / 100000) << "." << (BOOST_VERSION / 100 % 1000) << "." << (BOOST_VERSION % 100) << ")";

    if (!fZmqCtx) {
        throw std::runtime_error(tools::ToString("failed creating context, reason: ", zmq_strerror(errno)));
    }

    int numIoThreads = 1;
    std::string sessionName = "default";
    size_t segmentSize = 2ULL << 30;
    std::string allocationAlgorithm("rbtree_best_fit");
    if (config) {
        numIoThreads = config->GetProperty<int>("io-threads", numIoThreads);
        sessionName = config->GetProperty<std::string>("session", sessionName);
        segmentSize = config->GetProperty<size_t>("shm-segment-size", segmentSize);
        allocationAlgorithm = config->GetProperty<std::string>("shm-allocation", allocationAlgorithm);
    } else {
        LOG(debug) << "ProgOptions not available! Using defaults.";
    }

    if (allocationAlgorithm != "rbtree_best_fit" && allocationAlgorithm != "simple_seq_fit") {
        LOG(error) << "Provided shared memory allocation algorithm '" << allocationAlgorithm << "' is not supported. Supported are 'rbtree_best_fit'/'simple_seq_fit'";
        throw SharedMemoryError(tools::ToString("Provided shared memory allocation algorithm '", allocationAlgorithm, "' is not supported. Supported are 'rbtree_best_fit'/'simple_seq_fit'"));
    }

    try {
        if (zmq_ctx_set(fZmqCtx, ZMQ_IO_THREADS, numIoThreads) != 0) {
            LOG(error) << "failed configuring context, reason: " << zmq_strerror(errno);
        }

                 // Set the maximum number of allowed sockets on the context.
        if (zmq_ctx_set(fZmqCtx, ZMQ_MAX_SOCKETS, 10000) != 0) {
            LOG(error) << "failed configuring context, reason: " << zmq_strerror(errno);
        }

        fManager = std::make_unique<Manager>(sessionName, segmentSize, config);
    } catch (boost::interprocess::interprocess_exception& e) {
        LOG(error) << "Could not initialize shared memory transport: " << e.what();
        throw std::runtime_error(tools::ToString("Could not initialize shared memory transport: ", e.what()));
    } catch (const std::exception& e) {
        LOG(error) << "Could not initialize shared memory transport: " << e.what();
        throw std::runtime_error(tools::ToString("Could not initialize shared memory transport: ", e.what()));
    }
}

TransportFactory::~TransportFactory()
{
    LOG(debug) << "Destroying Shared Memory transport...";

    if (fZmqCtx) {
        while (true) {
            if (zmq_ctx_term(fZmqCtx) != 0) {
                if (errno == EINTR) {
                    LOG(debug) << "zmq_ctx_term interrupted by system call, retrying";
                    continue;
                } else {
                    fZmqCtx = nullptr;
                }
            }
            break;
        }
    } else {
        LOG(error) << "context not available for shutdown";
    }
}

MessagePtr TransportFactory::CreateMessage()
{
    return std::make_unique<Message>(*fManager, this);
}

MessagePtr TransportFactory::CreateMessage(Alignment alignment)
{
    return std::make_unique<Message>(*fManager, alignment, this);
}

MessagePtr TransportFactory::CreateMessage(size_t size)
{
    return std::make_unique<Message>(*fManager, size, this);
}

MessagePtr TransportFactory::CreateMessage(size_t size, Alignment alignment)
{
    return std::make_unique<Message>(*fManager, size, alignment, this);
}

MessagePtr TransportFactory::CreateMessage(void* data, size_t size, fair::mq::FreeFn* ffn, void* hint)
{
    return std::make_unique<Message>(*fManager, data, size, ffn, hint, this);
}

MessagePtr TransportFactory::CreateMessage(UnmanagedRegionPtr& region, void* data, size_t size, void* hint)
{
    return std::make_unique<Message>(*fManager, region, data, size, hint, this);
}

SocketPtr TransportFactory::CreateSocket(const std::string& type, const std::string& name)
{
    return std::make_unique<Socket>(*fManager, type, name, GetId(), fZmqCtx, this);
}

PollerPtr TransportFactory::CreatePoller(const std::vector<Channel>& channels) const
{
    return std::make_unique<Poller>(channels);
}

PollerPtr TransportFactory::CreatePoller(const std::vector<Channel*>& channels) const
{
    return std::make_unique<Poller>(channels);
}

PollerPtr TransportFactory::CreatePoller(const std::unordered_map<std::string, std::vector<Channel>>& channelsMap, const std::vector<std::string>& channelList) const
{
    return std::make_unique<Poller>(channelsMap, channelList);
}

UnmanagedRegionPtr TransportFactory::CreateUnmanagedRegion(size_t size, RegionCallback callback, const std::string& path, int flags, fair::mq::RegionConfig cfg)
{
    cfg.path = path;
    cfg.creationFlags = flags;
    return CreateUnmanagedRegion(size, callback, nullptr, std::move(cfg));
}

UnmanagedRegionPtr TransportFactory::CreateUnmanagedRegion(size_t size, RegionBulkCallback bulkCallback, const std::string& path, int flags, fair::mq::RegionConfig cfg)
{
    cfg.path = path;
    cfg.creationFlags = flags;
    return CreateUnmanagedRegion(size, nullptr, bulkCallback, std::move(cfg));
}

UnmanagedRegionPtr TransportFactory::CreateUnmanagedRegion(size_t size, int64_t userFlags, RegionCallback callback, const std::string& path, int flags, fair::mq::RegionConfig cfg)
{
    cfg.path = path;
    cfg.userFlags = userFlags;
    cfg.creationFlags = flags;
    return CreateUnmanagedRegion(size, callback, nullptr, std::move(cfg));
}

UnmanagedRegionPtr TransportFactory::CreateUnmanagedRegion(size_t size, int64_t userFlags, RegionBulkCallback bulkCallback, const std::string& path, int flags, fair::mq::RegionConfig cfg)
{
    cfg.path = path;
    cfg.userFlags = userFlags;
    cfg.creationFlags = flags;
    return CreateUnmanagedRegion(size, nullptr, bulkCallback, std::move(cfg));
}

UnmanagedRegionPtr TransportFactory::CreateUnmanagedRegion(size_t size, RegionCallback callback, RegionConfig cfg)
{
    return CreateUnmanagedRegion(size, callback, nullptr, std::move(cfg));
}
UnmanagedRegionPtr TransportFactory::CreateUnmanagedRegion(size_t size, RegionBulkCallback bulkCallback, RegionConfig cfg)
{
    return CreateUnmanagedRegion(size, nullptr, bulkCallback, std::move(cfg));
}

UnmanagedRegionPtr TransportFactory::CreateUnmanagedRegion(size_t size, RegionCallback callback, RegionBulkCallback bulkCallback, fair::mq::RegionConfig cfg)
{
    return std::make_unique<UnmanagedRegionImpl>(*fManager, size, callback, bulkCallback, std::move(cfg), this);
}

void TransportFactory::SubscribeToRegionEvents(RegionEventCallback callback) { fManager->SubscribeToRegionEvents(callback); }
bool TransportFactory::SubscribedToRegionEvents() { return fManager->SubscribedToRegionEvents(); }
void TransportFactory::UnsubscribeFromRegionEvents() { fManager->UnsubscribeFromRegionEvents(); }
std::vector<fair::mq::RegionInfo> TransportFactory::GetRegionInfo() { return fManager->GetRegionInfo(); }

}

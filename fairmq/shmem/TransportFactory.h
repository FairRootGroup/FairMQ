/********************************************************************************
 * Copyright (C) 2016-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SHMEM_TRANSPORTFACTORY_H_
#define FAIR_MQ_SHMEM_TRANSPORTFACTORY_H_

#include "Manager.h"
#include "Common.h"
#include "Message.h"
#include "Socket.h"
#include "Poller.h"
#include "UnmanagedRegion.h"

#include <FairMQTransportFactory.h>
#include <fairmq/ProgOptions.h>
#include <FairMQLogger.h>
#include <fairmq/Tools.h>

#include <boost/version.hpp>

#include <zmq.h>

#include <memory> // unique_ptr
#include <string>
#include <vector>

namespace fair
{
namespace mq
{
namespace shmem
{

class TransportFactory final : public fair::mq::TransportFactory
{
  public:
    TransportFactory(const std::string& id = "", const ProgOptions* config = nullptr)
        : fair::mq::TransportFactory(id)
        , fDeviceId(id)
        , fShmId()
        , fZMQContext(zmq_ctx_new())
        , fManager(nullptr)
    {
        int major, minor, patch;
        zmq_version(&major, &minor, &patch);
        LOG(debug) << "Transport: Using ZeroMQ (" << major << "." << minor << "." << patch << ") & "
                << "boost::interprocess (" << (BOOST_VERSION / 100000) << "." << (BOOST_VERSION / 100 % 1000) << "." << (BOOST_VERSION % 100) << ")";

        if (!fZMQContext) {
            throw std::runtime_error(tools::ToString("failed creating context, reason: ", zmq_strerror(errno)));
        }

        int numIoThreads = 1;
        std::string sessionName = "default";
        size_t segmentSize = 2000000000;
        bool autolaunchMonitor = false;
        bool throwOnBadAlloc = true;
        if (config) {
            numIoThreads = config->GetProperty<int>("io-threads", numIoThreads);
            sessionName = config->GetProperty<std::string>("session", sessionName);
            segmentSize = config->GetProperty<size_t>("shm-segment-size", segmentSize);
            autolaunchMonitor = config->GetProperty<bool>("shm-monitor", autolaunchMonitor);
            throwOnBadAlloc = config->GetProperty<bool>("shm-throw-bad-alloc", throwOnBadAlloc);
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

            fManager = tools::make_unique<Manager>(fShmId, fDeviceId, segmentSize, throwOnBadAlloc);
        } catch (boost::interprocess::interprocess_exception& e) {
            LOG(error) << "Could not initialize shared memory transport: " << e.what();
            throw std::runtime_error(tools::ToString("Could not initialize shared memory transport: ", e.what()));
        }
    }

    TransportFactory(const TransportFactory&) = delete;
    TransportFactory operator=(const TransportFactory&) = delete;

    MessagePtr CreateMessage() override
    {
        return tools::make_unique<Message>(*fManager, this);
    }

    MessagePtr CreateMessage(const size_t size) override
    {
        return tools::make_unique<Message>(*fManager, size, this);
    }

    MessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override
    {
        return tools::make_unique<Message>(*fManager, data, size, ffn, hint, this);
    }

    MessagePtr CreateMessage(UnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0) override
    {
        return tools::make_unique<Message>(*fManager, region, data, size, hint, this);
    }

    SocketPtr CreateSocket(const std::string& type, const std::string& name) override
    {
        return tools::make_unique<Socket>(*fManager, type, name, GetId(), fZMQContext, this);
    }

    PollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const override
    {
        return tools::make_unique<Poller>(channels);
    }

    PollerPtr CreatePoller(const std::vector<FairMQChannel*>& channels) const override
    {
        return tools::make_unique<Poller>(channels);
    }

    PollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const override
    {
        return tools::make_unique<Poller>(channelsMap, channelList);
    }

    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, RegionCallback callback = nullptr, const std::string& path = "", int flags = 0) override
    {
        return CreateUnmanagedRegion(size, 0, callback, nullptr, path, flags);
    }

    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, RegionBulkCallback bulkCallback = nullptr, const std::string& path = "", int flags = 0) override
    {
        return CreateUnmanagedRegion(size, 0, nullptr, bulkCallback, path, flags);
    }

    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, int64_t userFlags, RegionCallback callback = nullptr, const std::string& path = "", int flags = 0) override
    {
        return CreateUnmanagedRegion(size, userFlags, callback, nullptr, path, flags);
    }

    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, int64_t userFlags, RegionBulkCallback bulkCallback = nullptr, const std::string& path = "", int flags = 0) override
    {
        return CreateUnmanagedRegion(size, userFlags, nullptr, bulkCallback, path, flags);
    }

    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, int64_t userFlags, RegionCallback callback, RegionBulkCallback bulkCallback, const std::string& path, int flags)
    {
        return tools::make_unique<UnmanagedRegion>(*fManager, size, userFlags, callback, bulkCallback, path, flags, this);
    }

    void SubscribeToRegionEvents(RegionEventCallback callback) override { fManager->SubscribeToRegionEvents(callback); }
    bool SubscribedToRegionEvents() override { return fManager->SubscribedToRegionEvents(); }
    void UnsubscribeFromRegionEvents() override { fManager->UnsubscribeFromRegionEvents(); }
    std::vector<fair::mq::RegionInfo> GetRegionInfo() override { return fManager->GetRegionInfo(); }

    Transport GetType() const override { return fair::mq::Transport::SHM; }

    void Interrupt() override { fManager->Interrupt(); }
    void Resume() override { fManager->Resume(); }
    void Reset() override { fManager->Reset(); }

    ~TransportFactory() override
    {
        LOG(debug) << "Destroying Shared Memory transport...";

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

  private:
    std::string fDeviceId;
    std::string fShmId;
    void* fZMQContext;
    std::unique_ptr<Manager> fManager;
};

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_TRANSPORTFACTORY_H_ */

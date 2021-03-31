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
#include <fairmq/tools/Strings.h>

#include <boost/version.hpp>

#include <zmq.h>

#include <memory> // unique_ptr, make_unique
#include <string>
#include <vector>
#include <stdexcept>

namespace fair::mq::shmem
{

class TransportFactory final : public fair::mq::TransportFactory
{
  public:
    TransportFactory(const std::string& deviceId = "", const ProgOptions* config = nullptr)
        : fair::mq::TransportFactory(deviceId)
        , fZmqCtx(zmq_ctx_new())
        , fManager(nullptr)
    {
        int major, minor, patch;
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

            fManager = std::make_unique<Manager>(sessionName, deviceId, segmentSize, config);
        } catch (boost::interprocess::interprocess_exception& e) {
            LOG(error) << "Could not initialize shared memory transport: " << e.what();
            throw std::runtime_error(tools::ToString("Could not initialize shared memory transport: ", e.what()));
        } catch (const std::exception& e) {
            LOG(error) << "Could not initialize shared memory transport: " << e.what();
            throw std::runtime_error(tools::ToString("Could not initialize shared memory transport: ", e.what()));
        }
    }

    TransportFactory(const TransportFactory&) = delete;
    TransportFactory operator=(const TransportFactory&) = delete;

    MessagePtr CreateMessage() override
    {
        return std::make_unique<Message>(*fManager, this);
    }

    MessagePtr CreateMessage(Alignment alignment) override
    {
        return std::make_unique<Message>(*fManager, alignment, this);
    }

    MessagePtr CreateMessage(const size_t size) override
    {
        return std::make_unique<Message>(*fManager, size, this);
    }

    MessagePtr CreateMessage(const size_t size, Alignment alignment) override
    {
        return std::make_unique<Message>(*fManager, size, alignment, this);
    }

    MessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override
    {
        return std::make_unique<Message>(*fManager, data, size, ffn, hint, this);
    }

    MessagePtr CreateMessage(UnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0) override
    {
        return std::make_unique<Message>(*fManager, region, data, size, hint, this);
    }

    SocketPtr CreateSocket(const std::string& type, const std::string& name) override
    {
        return std::make_unique<Socket>(*fManager, type, name, GetId(), fZmqCtx, this);
    }

    PollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const override
    {
        return std::make_unique<Poller>(channels);
    }

    PollerPtr CreatePoller(const std::vector<FairMQChannel*>& channels) const override
    {
        return std::make_unique<Poller>(channels);
    }

    PollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const override
    {
        return std::make_unique<Poller>(channelsMap, channelList);
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
        return std::make_unique<UnmanagedRegion>(*fManager, size, userFlags, callback, bulkCallback, path, flags, this);
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

  private:
    void* fZmqCtx;
    std::unique_ptr<Manager> fManager;
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_TRANSPORTFACTORY_H_ */

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTransportFactoryZMQ.h
 *
 * @since 2014-01-20
 * @author: A. Rybalchenko
 */

#ifndef FAIRMQTRANSPORTFACTORYZMQ_H_
#define FAIRMQTRANSPORTFACTORYZMQ_H_

#include <fairmq/zeromq/Context.h>
#include <fairmq/Tools.h>
#include <fairmq/ProgOptions.h>
#include <FairMQTransportFactory.h>
#include "FairMQMessageZMQ.h"
#include "FairMQSocketZMQ.h"
#include "FairMQPollerZMQ.h"
#include "FairMQUnmanagedRegionZMQ.h"

#include <memory> // unique_ptr
#include <string>
#include <vector>

class FairMQTransportFactoryZMQ final : public FairMQTransportFactory
{
  public:
    FairMQTransportFactoryZMQ(const std::string& id = "", const fair::mq::ProgOptions* config = nullptr)
        : FairMQTransportFactory(id)
        , fCtx(nullptr)
    {
        int major, minor, patch;
        zmq_version(&major, &minor, &patch);
        LOG(debug) << "Transport: Using ZeroMQ library, version: " << major << "." << minor << "." << patch;

        if (config) {
            fCtx = fair::mq::tools::make_unique<fair::mq::zmq::Context>(config->GetProperty<int>("io-threads", 1));
        } else {
            LOG(debug) << "fair::mq::ProgOptions not available! Using defaults.";
            fCtx = fair::mq::tools::make_unique<fair::mq::zmq::Context>(1);
        }
    }

    FairMQTransportFactoryZMQ(const FairMQTransportFactoryZMQ&) = delete;
    FairMQTransportFactoryZMQ operator=(const FairMQTransportFactoryZMQ&) = delete;

    FairMQMessagePtr CreateMessage() override { return fair::mq::tools::make_unique<FairMQMessageZMQ>(this); }
    FairMQMessagePtr CreateMessage(const size_t size) override { return fair::mq::tools::make_unique<FairMQMessageZMQ>(size, this); }
    FairMQMessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override { return fair::mq::tools::make_unique<FairMQMessageZMQ>(data, size, ffn, hint, this); }
    FairMQMessagePtr CreateMessage(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0) override { return fair::mq::tools::make_unique<FairMQMessageZMQ>(region, data, size, hint, this); }

    FairMQSocketPtr CreateSocket(const std::string& type, const std::string& name) override { return fair::mq::tools::make_unique<FairMQSocketZMQ>(*fCtx, type, name, GetId(), this); }

    FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const override { return fair::mq::tools::make_unique<FairMQPollerZMQ>(channels); }
    FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel*>& channels) const override { return fair::mq::tools::make_unique<FairMQPollerZMQ>(channels); }
    FairMQPollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const override { return fair::mq::tools::make_unique<FairMQPollerZMQ>(channelsMap, channelList); }

    FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback, const std::string& path = "", int flags = 0) override { return CreateUnmanagedRegion(size, 0, callback, nullptr, path, flags); }
    FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, FairMQRegionBulkCallback bulkCallback, const std::string& path = "", int flags = 0) override { return CreateUnmanagedRegion(size, 0, nullptr, bulkCallback, path, flags); }
    FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, const int64_t userFlags, FairMQRegionCallback callback, const std::string& path = "", int flags = 0) override { return CreateUnmanagedRegion(size, userFlags, callback, nullptr, path, flags); }
    FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, const int64_t userFlags, FairMQRegionBulkCallback bulkCallback, const std::string& path = "", int flags = 0) override { return CreateUnmanagedRegion(size, userFlags, nullptr, bulkCallback, path, flags); }
    FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, const int64_t userFlags, FairMQRegionCallback callback, FairMQRegionBulkCallback bulkCallback, const std::string& path = "", int flags = 0)
    {
        auto ptr = std::unique_ptr<FairMQUnmanagedRegion>(new FairMQUnmanagedRegionZMQ(*fCtx, size, userFlags, callback, bulkCallback, path, flags, this));
        auto zPtr = static_cast<FairMQUnmanagedRegionZMQ*>(ptr.get());
        fCtx->AddRegion(zPtr->GetId(), zPtr->GetData(), zPtr->GetSize(), zPtr->GetUserFlags(), fair::mq::RegionEvent::created);
        return ptr;
    }

    void SubscribeToRegionEvents(FairMQRegionEventCallback callback) override { fCtx->SubscribeToRegionEvents(callback); }
    bool SubscribedToRegionEvents() override { return fCtx->SubscribedToRegionEvents(); }
    void UnsubscribeFromRegionEvents() override { fCtx->UnsubscribeFromRegionEvents(); }
    std::vector<fair::mq::RegionInfo> GetRegionInfo() override { return fCtx->GetRegionInfo(); }

    fair::mq::Transport GetType() const override { return fair::mq::Transport::ZMQ; }

    void Interrupt() override { fCtx->Interrupt(); }
    void Resume() override { fCtx->Resume(); }
    void Reset() override { fCtx->Reset(); }

    ~FairMQTransportFactoryZMQ() override { LOG(debug) << "Destroying ZeroMQ transport..."; }

  private:
    std::unique_ptr<fair::mq::zmq::Context> fCtx;
};

#endif /* FAIRMQTRANSPORTFACTORYZMQ_H_ */

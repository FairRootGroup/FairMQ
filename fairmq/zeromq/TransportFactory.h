/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_ZMQ_TRANSPORTFACTORY_H
#define FAIR_MQ_ZMQ_TRANSPORTFACTORY_H

#include <fairmq/zeromq/Context.h>
#include <fairmq/zeromq/Message.h>
#include <fairmq/zeromq/Socket.h>
#include <fairmq/zeromq/Poller.h>
#include <fairmq/zeromq/UnmanagedRegion.h>
#include <fairmq/Tools.h>
#include <FairMQTransportFactory.h>
#include <fairmq/ProgOptions.h>

#include <memory> // unique_ptr
#include <string>
#include <vector>

namespace fair
{
namespace mq
{
namespace zmq
{

class TransportFactory final : public FairMQTransportFactory
{
  public:
    TransportFactory(const std::string& id = "", const ProgOptions* config = nullptr)
        : FairMQTransportFactory(id)
        , fCtx(nullptr)
    {
        int major, minor, patch;
        zmq_version(&major, &minor, &patch);
        LOG(debug) << "Transport: Using ZeroMQ library, version: " << major << "." << minor << "." << patch;

        if (config) {
            fCtx = tools::make_unique<Context>(config->GetProperty<int>("io-threads", 1));
        } else {
            LOG(debug) << "fair::mq::ProgOptions not available! Using defaults.";
            fCtx = tools::make_unique<Context>(1);
        }
    }

    TransportFactory(const TransportFactory&) = delete;
    TransportFactory operator=(const TransportFactory&) = delete;

    MessagePtr CreateMessage() override
    {
        return tools::make_unique<Message>(this);
    }

    MessagePtr CreateMessage(Alignment alignment) override
    {
        return tools::make_unique<Message>(alignment, this);
    }

    MessagePtr CreateMessage(const size_t size) override
    {
        return tools::make_unique<Message>(size, this);
    }

    MessagePtr CreateMessage(const size_t size, Alignment alignment) override
    {
        return tools::make_unique<Message>(size, alignment, this);
    }

    MessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override
    {
        return tools::make_unique<Message>(data, size, ffn, hint, this);
    }

    MessagePtr CreateMessage(UnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0) override
    {
        return tools::make_unique<Message>(region, data, size, hint, this);
    }

    SocketPtr CreateSocket(const std::string& type, const std::string& name) override
    {
        return tools::make_unique<Socket>(*fCtx, type, name, GetId(), this);
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

    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, RegionCallback callback, const std::string& path = "", int flags = 0) override
    {
        return CreateUnmanagedRegion(size, 0, callback, nullptr, path, flags);
    }

    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, RegionBulkCallback bulkCallback, const std::string& path = "", int flags = 0) override
    {
        return CreateUnmanagedRegion(size, 0, nullptr, bulkCallback, path, flags);
    }

    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, const int64_t userFlags, RegionCallback callback, const std::string& path = "", int flags = 0) override
    {
        return CreateUnmanagedRegion(size, userFlags, callback, nullptr, path, flags);
    }

    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, const int64_t userFlags, RegionBulkCallback bulkCallback, const std::string& path = "", int flags = 0) override
    {
        return CreateUnmanagedRegion(size, userFlags, nullptr, bulkCallback, path, flags);
    }

    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, const int64_t userFlags, RegionCallback callback, RegionBulkCallback bulkCallback, const std::string&, int)
    {
        UnmanagedRegionPtr ptr = tools::make_unique<UnmanagedRegion>(*fCtx, size, userFlags, callback, bulkCallback, this);
        auto zPtr = static_cast<UnmanagedRegion*>(ptr.get());
        fCtx->AddRegion(false, zPtr->GetId(), zPtr->GetData(), zPtr->GetSize(), zPtr->GetUserFlags(), RegionEvent::created);
        return ptr;
    }

    void SubscribeToRegionEvents(RegionEventCallback callback) override { fCtx->SubscribeToRegionEvents(callback); }
    bool SubscribedToRegionEvents() override { return fCtx->SubscribedToRegionEvents(); }
    void UnsubscribeFromRegionEvents() override { fCtx->UnsubscribeFromRegionEvents(); }
    std::vector<RegionInfo> GetRegionInfo() override { return fCtx->GetRegionInfo(); }

    Transport GetType() const override { return Transport::ZMQ; }

    void Interrupt() override { fCtx->Interrupt(); }
    void Resume() override { fCtx->Resume(); }
    void Reset() override { fCtx->Reset(); }

    ~TransportFactory() override { LOG(debug) << "Destroying ZeroMQ transport..."; }

  private:
    std::unique_ptr<Context> fCtx;
};

}   // namespace zmq
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_ZMQ_TRANSPORTFACTORY_H */

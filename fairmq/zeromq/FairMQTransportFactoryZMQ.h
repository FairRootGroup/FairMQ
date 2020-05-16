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
#include <fairmq/ProgOptions.h>
#include "FairMQTransportFactory.h"
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
    FairMQTransportFactoryZMQ(const std::string& id = "", const fair::mq::ProgOptions* config = nullptr);
    FairMQTransportFactoryZMQ(const FairMQTransportFactoryZMQ&) = delete;
    FairMQTransportFactoryZMQ operator=(const FairMQTransportFactoryZMQ&) = delete;

    FairMQMessagePtr CreateMessage() override;
    FairMQMessagePtr CreateMessage(const size_t size) override;
    FairMQMessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override;
    FairMQMessagePtr CreateMessage(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0) override;

    FairMQSocketPtr CreateSocket(const std::string& type, const std::string& name) override;

    FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel*>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const override;

    FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback, const std::string& path = "", int flags = 0) override;
    FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, FairMQRegionBulkCallback bulkCallback, const std::string& path = "", int flags = 0) override;
    FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, const int64_t userFlags, FairMQRegionCallback callback, const std::string& path = "", int flags = 0) override;
    FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, const int64_t userFlags, FairMQRegionBulkCallback bulkCallback, const std::string& path = "", int flags = 0) override;
    FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, const int64_t userFlags, FairMQRegionCallback callback, FairMQRegionBulkCallback bulkCallback, const std::string& path = "", int flags = 0);

    void SubscribeToRegionEvents(FairMQRegionEventCallback callback) override { fCtx->SubscribeToRegionEvents(callback); }
    bool SubscribedToRegionEvents() override { return fCtx->SubscribedToRegionEvents(); }
    void UnsubscribeFromRegionEvents() override { fCtx->UnsubscribeFromRegionEvents(); }
    std::vector<fair::mq::RegionInfo> GetRegionInfo() override { return fCtx->GetRegionInfo(); }

    fair::mq::Transport GetType() const override { return fair::mq::Transport::ZMQ; }

    void Interrupt() override { fCtx->Interrupt(); }
    void Resume() override { fCtx->Resume(); }
    void Reset() override { fCtx->Reset(); }

    ~FairMQTransportFactoryZMQ() override;

  private:
    std::unique_ptr<fair::mq::zmq::Context> fCtx;
};

#endif /* FAIRMQTRANSPORTFACTORYZMQ_H_ */

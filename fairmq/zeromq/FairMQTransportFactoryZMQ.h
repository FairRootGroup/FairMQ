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

#include <vector>
#include <string>

#include "FairMQTransportFactory.h"
#include "FairMQMessageZMQ.h"
#include "FairMQSocketZMQ.h"
#include "FairMQPollerZMQ.h"
#include "FairMQUnmanagedRegionZMQ.h"
#include <options/FairMQProgOptions.h>

class FairMQTransportFactoryZMQ final : public FairMQTransportFactory
{
  public:
    FairMQTransportFactoryZMQ(const std::string& id = "", const FairMQProgOptions* config = nullptr);
    FairMQTransportFactoryZMQ(const FairMQTransportFactoryZMQ&) = delete;
    FairMQTransportFactoryZMQ operator=(const FairMQTransportFactoryZMQ&) = delete;

    ~FairMQTransportFactoryZMQ() override;

    FairMQMessagePtr CreateMessage() override;
    FairMQMessagePtr CreateMessage(const size_t size) override;
    FairMQMessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override;
    FairMQMessagePtr CreateMessage(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0) override;

    FairMQSocketPtr CreateSocket(const std::string& type, const std::string& name) override;

    FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel*>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const override;

    FairMQUnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback, const std::string& path = "", int flags = 0) const override;

    fair::mq::Transport GetType() const override;

    void Interrupt() override { FairMQSocketZMQ::Interrupt(); }
    void Resume() override { FairMQSocketZMQ::Resume(); }
    void Reset() override {}

  private:
    static fair::mq::Transport fTransportType;
    void* fContext;
};

#endif /* FAIRMQTRANSPORTFACTORYZMQ_H_ */

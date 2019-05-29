/********************************************************************************
 * Copyright (C) 2014-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQTRANSPORTFACTORYNN_H_
#define FAIRMQTRANSPORTFACTORYNN_H_

#include "FairMQTransportFactory.h"
#include "FairMQMessageNN.h"
#include "FairMQSocketNN.h"
#include "FairMQPollerNN.h"
#include "FairMQUnmanagedRegionNN.h"
#include <fairmq/ProgOptions.h>

#include <vector>
#include <string>

class FairMQTransportFactoryNN final : public FairMQTransportFactory
{
  public:
    FairMQTransportFactoryNN(const std::string& id = "", const fair::mq::ProgOptions* config = nullptr);
    ~FairMQTransportFactoryNN() override;

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

    void Interrupt() override { FairMQSocketNN::Interrupt(); }
    void Resume() override { FairMQSocketNN::Resume(); }
    void Reset() override;

  private:
    static fair::mq::Transport fTransportType;
    mutable std::vector<FairMQSocket*> fSockets;
};

#endif /* FAIRMQTRANSPORTFACTORYNN_H_ */

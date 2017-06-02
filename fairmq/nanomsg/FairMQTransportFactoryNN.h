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
#include "FairMQRegionNN.h"
#include <options/FairMQProgOptions.h>

#include <vector>
#include <string>

class FairMQTransportFactoryNN : public FairMQTransportFactory
{
  public:
    FairMQTransportFactoryNN(const std::string& id = "", const FairMQProgOptions* config = nullptr);
    ~FairMQTransportFactoryNN() override;

    FairMQMessagePtr CreateMessage() const override;
    FairMQMessagePtr CreateMessage(const size_t size) const override;
    FairMQMessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) const override;
    FairMQMessagePtr CreateMessage(FairMQRegionPtr& region, void* data, const size_t size) const override;

    FairMQSocketPtr CreateSocket(const std::string& type, const std::string& name) const override;

    FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::vector<const FairMQChannel*>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const override;
    FairMQPollerPtr CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket) const override;

    FairMQRegionPtr CreateRegion(const size_t size) const override;

    FairMQ::Transport GetType() const override;

  private:
    static FairMQ::Transport fTransportType;
};

#endif /* FAIRMQTRANSPORTFACTORYNN_H_ */

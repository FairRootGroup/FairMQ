/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQTRANSPORTFACTORYSHM_H_
#define FAIRMQTRANSPORTFACTORYSHM_H_

#include <vector>
#include <string>

#include "FairMQTransportFactory.h"
#include "FairMQMessageSHM.h"
#include "FairMQSocketSHM.h"
#include "FairMQPollerSHM.h"

class FairMQTransportFactorySHM : public FairMQTransportFactory
{
  public:
    FairMQTransportFactorySHM();

    void Initialize(const FairMQProgOptions* config) override;

    FairMQMessagePtr CreateMessage() const override;
    FairMQMessagePtr CreateMessage(const size_t size) const override;
    FairMQMessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) const override;

    virtual FairMQSocketPtr CreateSocket(const std::string& type, const std::string& name, const std::string& id = "") const;

    FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const override;
    FairMQPollerPtr CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket) const override;

    FairMQ::Transport GetType() const override;

    void Shutdown() override;
    void Terminate() override;

    ~FairMQTransportFactorySHM() override {};

  private:
    static FairMQ::Transport fTransportType;
    void* fContext;
};

#endif /* FAIRMQTRANSPORTFACTORYSHM_H_ */

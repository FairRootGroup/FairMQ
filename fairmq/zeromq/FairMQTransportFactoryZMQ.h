/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
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
#include <options/FairMQProgOptions.h>

class FairMQTransportFactoryZMQ : public FairMQTransportFactory
{
  public:
    FairMQTransportFactoryZMQ(const std::string& id = "", const FairMQProgOptions* config = nullptr);
    ~FairMQTransportFactoryZMQ() override;

    FairMQMessagePtr CreateMessage() const override;
    FairMQMessagePtr CreateMessage(const size_t size) const override;
    FairMQMessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) const override;

    FairMQSocketPtr CreateSocket(const std::string& type, const std::string& name) const override;

    FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::vector<const FairMQChannel*>& channels) const override;
    FairMQPollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const override;
    FairMQPollerPtr CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket) const override;

    FairMQ::Transport GetType() const override;
  private:
    static FairMQ::Transport fTransportType;
    void* fContext;
};

#endif /* FAIRMQTRANSPORTFACTORYZMQ_H_ */

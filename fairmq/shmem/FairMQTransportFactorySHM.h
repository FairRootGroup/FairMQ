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
#include "FairMQContextSHM.h"
#include "FairMQMessageSHM.h"
#include "FairMQSocketSHM.h"
#include "FairMQPollerSHM.h"

class FairMQTransportFactorySHM : public FairMQTransportFactory
{
  public:
    FairMQTransportFactorySHM();

    virtual FairMQMessagePtr CreateMessage() const;
    virtual FairMQMessagePtr CreateMessage(const size_t size) const;
    virtual FairMQMessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = NULL) const;

    virtual FairMQSocketPtr CreateSocket(const std::string& type, const std::string& name, const int numIoThreads, const std::string& id = "") const;

    virtual FairMQPollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const;
    virtual FairMQPollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const;
    virtual FairMQPollerPtr CreatePoller(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket) const;

    virtual FairMQ::Transport GetType() const;

    virtual ~FairMQTransportFactorySHM() {};
};

#endif /* FAIRMQTRANSPORTFACTORYSHM_H_ */

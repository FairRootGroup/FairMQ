/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQPOLLERSHM_H_
#define FAIRMQPOLLERSHM_H_

#include <vector>
#include <unordered_map>
#include <initializer_list>

#include <zmq.h>

#include "FairMQPoller.h"
#include "FairMQChannel.h"
#include "FairMQTransportFactorySHM.h"

class FairMQChannel;

class FairMQPollerSHM : public FairMQPoller
{
    friend class FairMQChannel;
    friend class FairMQTransportFactorySHM;

  public:
    FairMQPollerSHM(const std::vector<FairMQChannel>& channels);
    FairMQPollerSHM(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList);
    FairMQPollerSHM(const FairMQPollerSHM&) = delete;
    FairMQPollerSHM operator=(const FairMQPollerSHM&) = delete;

    virtual void Poll(const int timeout);
    virtual bool CheckInput(const int index);
    virtual bool CheckOutput(const int index);
    virtual bool CheckInput(const std::string channelKey, const int index);
    virtual bool CheckOutput(const std::string channelKey, const int index);

    virtual ~FairMQPollerSHM();

  private:
    FairMQPollerSHM(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket);

    zmq_pollitem_t* items;
    int fNumItems;

    std::unordered_map<std::string, int> fOffsetMap;
};

#endif /* FAIRMQPOLLERSHM_H_ */
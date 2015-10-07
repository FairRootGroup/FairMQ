/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQPollerZMQ.h
 *
 * @since 2014-01-23
 * @author A. Rybalchenko
 */

#ifndef FAIRMQPOLLERZMQ_H_
#define FAIRMQPOLLERZMQ_H_

#include <vector>
#include <unordered_map>
#include <initializer_list>

#include "FairMQPoller.h"
#include "FairMQChannel.h"
#include "FairMQTransportFactoryZMQ.h"

class FairMQChannel;

class FairMQPollerZMQ : public FairMQPoller
{
    friend class FairMQChannel;
    friend class FairMQTransportFactoryZMQ;

  public:
    FairMQPollerZMQ(const std::vector<FairMQChannel>& channels);
    FairMQPollerZMQ(std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, std::initializer_list<std::string> channelList);

    virtual void Poll(const int timeout);
    virtual bool CheckInput(const int index);
    virtual bool CheckOutput(const int index);
    virtual bool CheckInput(const std::string channelKey, const int index);
    virtual bool CheckOutput(const std::string channelKey, const int index);

    virtual ~FairMQPollerZMQ();

  private:
    FairMQPollerZMQ(FairMQSocket& cmdSocket, FairMQSocket& dataSocket);

    zmq_pollitem_t* items;
    int fNumItems;

    std::unordered_map<std::string,int> fOffsetMap;

    /// Copy Constructor
    FairMQPollerZMQ(const FairMQPollerZMQ&);
    FairMQPollerZMQ operator=(const FairMQPollerZMQ&);
};

#endif /* FAIRMQPOLLERZMQ_H_ */
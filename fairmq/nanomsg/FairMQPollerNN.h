/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQPollerNN.h
 *
 * @since 2014-01-23
 * @author A. Rybalchenko
 */

#ifndef FAIRMQPOLLERNN_H_
#define FAIRMQPOLLERNN_H_

#include <vector>
#include <unordered_map>
#include <initializer_list>

#include "FairMQPoller.h"
#include "FairMQChannel.h"
#include "FairMQTransportFactoryNN.h"

class FairMQChannel;

class FairMQPollerNN : public FairMQPoller
{
    friend class FairMQChannel;
    friend class FairMQTransportFactoryNN;

  public:
    FairMQPollerNN(const std::vector<FairMQChannel>& channels);
    FairMQPollerNN(std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, std::initializer_list<std::string> channelList);
    FairMQPollerNN(const FairMQPollerNN&) = delete;
    FairMQPollerNN operator=(const FairMQPollerNN&) = delete;

    virtual void Poll(const int timeout);
    virtual bool CheckInput(const int index);
    virtual bool CheckOutput(const int index);
    virtual bool CheckInput(const std::string channelKey, const int index);
    virtual bool CheckOutput(const std::string channelKey, const int index);

    virtual ~FairMQPollerNN();

  private:
    FairMQPollerNN(FairMQSocket& cmdSocket, FairMQSocket& dataSocket);

    nn_pollfd* items;
    int fNumItems;

    std::unordered_map<std::string, int> fOffsetMap;
};

#endif /* FAIRMQPOLLERNN_H_ */
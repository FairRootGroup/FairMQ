/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
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

#include <zmq.h>

#include "FairMQPoller.h"
#include "FairMQChannel.h"
#include "FairMQTransportFactoryZMQ.h"

class FairMQChannel;

class FairMQPollerZMQ final : public FairMQPoller
{
    friend class FairMQChannel;
    friend class FairMQTransportFactoryZMQ;

  public:
    FairMQPollerZMQ(const std::vector<FairMQChannel>& channels);
    FairMQPollerZMQ(const std::vector<FairMQChannel*>& channels);
    FairMQPollerZMQ(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList);

    FairMQPollerZMQ(const FairMQPollerZMQ&) = delete;
    FairMQPollerZMQ operator=(const FairMQPollerZMQ&) = delete;

    void SetItemEvents(zmq_pollitem_t& item, const int type);

    void Poll(const int timeout) override;
    bool CheckInput(const int index) override;
    bool CheckOutput(const int index) override;
    bool CheckInput(const std::string& channelKey, const int index) override;
    bool CheckOutput(const std::string& channelKey, const int index) override;

    ~FairMQPollerZMQ() override;

  private:
    zmq_pollitem_t* fItems;
    int fNumItems;

    std::unordered_map<std::string, int> fOffsetMap;
};

#endif /* FAIRMQPOLLERZMQ_H_ */

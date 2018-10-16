/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQPOLLERSHM_H_
#define FAIRMQPOLLERSHM_H_

#include <vector>
#include <unordered_map>

#include <zmq.h>

#include "FairMQPoller.h"
#include "FairMQChannel.h"
#include "FairMQTransportFactorySHM.h"

class FairMQChannel;

class FairMQPollerSHM final : public FairMQPoller
{
    friend class FairMQChannel;
    friend class FairMQTransportFactorySHM;

  public:
    FairMQPollerSHM(const std::vector<FairMQChannel>& channels);
    FairMQPollerSHM(const std::vector<const FairMQChannel*>& channels);
    FairMQPollerSHM(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList);

    FairMQPollerSHM(const FairMQPollerSHM&) = delete;
    FairMQPollerSHM operator=(const FairMQPollerSHM&) = delete;

    void SetItemEvents(zmq_pollitem_t& item, const int type);

    void Poll(const int timeout) override;
    bool CheckInput(const int index) override;
    bool CheckOutput(const int index) override;
    bool CheckInput(const std::string& channelKey, const int index) override;
    bool CheckOutput(const std::string& channelKey, const int index) override;

    ~FairMQPollerSHM() override;

  private:
    zmq_pollitem_t* fItems;
    int fNumItems;

    std::unordered_map<std::string, int> fOffsetMap;
};

#endif /* FAIRMQPOLLERSHM_H_ */
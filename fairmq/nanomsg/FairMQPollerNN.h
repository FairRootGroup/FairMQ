/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
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
struct nn_pollfd;

class FairMQPollerNN final : public FairMQPoller
{
    friend class FairMQChannel;
    friend class FairMQTransportFactoryNN;

  public:
    FairMQPollerNN(const std::vector<FairMQChannel>& channels);
    FairMQPollerNN(const std::vector<FairMQChannel*>& channels);
    FairMQPollerNN(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList);

    FairMQPollerNN(const FairMQPollerNN&) = delete;
    FairMQPollerNN operator=(const FairMQPollerNN&) = delete;

    void SetItemEvents(nn_pollfd& item, const int type);

    void Poll(const int timeout) override;
    bool CheckInput(const int index) override;
    bool CheckOutput(const int index) override;
    bool CheckInput(const std::string& channelKey, const int index) override;
    bool CheckOutput(const std::string& channelKey, const int index) override;

    ~FairMQPollerNN() override;

  private:
    nn_pollfd* fItems;
    int fNumItems;

    std::unordered_map<std::string, int> fOffsetMap;
};

#endif /* FAIRMQPOLLERNN_H_ */

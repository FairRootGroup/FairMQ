/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_SHMEM_POLLER_H_
#define FAIR_MQ_SHMEM_POLLER_H_

#include <zmq.h>

#include <FairMQPoller.h>
#include <FairMQChannel.h>

#include <vector>
#include <unordered_map>

class FairMQChannel;

namespace fair
{
namespace mq
{
namespace shmem
{

class Poller final : public fair::mq::Poller
{
  public:
    Poller(const std::vector<FairMQChannel>& channels);
    Poller(const std::vector<FairMQChannel*>& channels);
    Poller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList);

    Poller(const Poller&) = delete;
    Poller operator=(const Poller&) = delete;

    void SetItemEvents(zmq_pollitem_t& item, const int type);

    void Poll(const int timeout) override;
    bool CheckInput(const int index) override;
    bool CheckOutput(const int index) override;
    bool CheckInput(const std::string& channelKey, const int index) override;
    bool CheckOutput(const std::string& channelKey, const int index) override;

    ~Poller() override;

  private:
    zmq_pollitem_t* fItems;
    int fNumItems;

    std::unordered_map<std::string, int> fOffsetMap;
};

}
}
}

#endif /* FAIR_MQ_SHMEM_POLLER_H_ */

/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_OFI_POLLER_H
#define FAIR_MQ_OFI_POLLER_H

#include <FairMQChannel.h>
#include <FairMQPoller.h>
#include <FairMQSocket.h>

#include <vector>
#include <unordered_map>

#include <zmq.h>

namespace fair
{
namespace mq
{
namespace ofi
{

class TransportFactory;

/**
 * @class Poller Poller.h <fairmq/ofi/Poller.h>
 * @brief 
 *
 * @todo TODO insert long description
 */
class Poller final : public FairMQPoller
{
    friend class FairMQChannel;
    friend class TransportFactory;

  public:
    Poller(const std::vector<FairMQChannel>& channels);
    Poller(const std::vector<const FairMQChannel*>& channels);
    Poller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList);

    Poller(const Poller&) = delete;
    Poller operator=(const Poller&) = delete;

    auto SetItemEvents(zmq_pollitem_t& item, const int type) -> void;

    auto Poll(const int timeout) -> void override;
    auto CheckInput(const int index) -> bool override;
    auto CheckOutput(const int index) -> bool override;
    auto CheckInput(const std::string channelKey, const int index) -> bool override;
    auto CheckOutput(const std::string channelKey, const int index) -> bool override;

    ~Poller() override;

  private:
    zmq_pollitem_t* fItems;
    int fNumItems;

    std::unordered_map<std::string, int> fOffsetMap;
}; /* class Poller */

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_OFI_POLLER_H */

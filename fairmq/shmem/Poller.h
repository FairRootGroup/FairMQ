/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_SHMEM_POLLER_H_
#define FAIR_MQ_SHMEM_POLLER_H_

#include "Socket.h"
#include <fairmq/Tools.h>
#include <FairMQChannel.h>
#include <FairMQLogger.h>
#include <FairMQPoller.h>

#include <zmq.h>

#include <unordered_map>
#include <vector>

class FairMQChannel;

namespace fair {
namespace mq {
namespace shmem {

class Poller final : public fair::mq::Poller
{
  public:
    Poller(const std::vector<FairMQChannel>& channels)
        : fItems()
        , fNumItems(0)
        , fOffsetMap()
    {
        fNumItems = channels.size();
        fItems = new zmq_pollitem_t[fNumItems];

        for (int i = 0; i < fNumItems; ++i) {
            fItems[i].socket = static_cast<const Socket*>(&(channels.at(i).GetSocket()))->GetSocket();
            fItems[i].fd = 0;
            fItems[i].revents = 0;

            int type = 0;
            size_t size = sizeof(type);
            zmq_getsockopt(static_cast<const Socket*>(&(channels.at(i).GetSocket()))->GetSocket(), ZMQ_TYPE, &type, &size);

            SetItemEvents(fItems[i], type);
        }
    }

    Poller(const std::vector<FairMQChannel*>& channels)
        : fItems()
        , fNumItems(0)
        , fOffsetMap()
    {
        fNumItems = channels.size();
        fItems = new zmq_pollitem_t[fNumItems];

        for (int i = 0; i < fNumItems; ++i) {
            fItems[i].socket = static_cast<const Socket*>(&(channels.at(i)->GetSocket()))->GetSocket();
            fItems[i].fd = 0;
            fItems[i].revents = 0;

            int type = 0;
            size_t size = sizeof(type);
            zmq_getsockopt(static_cast<const Socket*>(&(channels.at(i)->GetSocket()))->GetSocket(), ZMQ_TYPE, &type, &size);

            SetItemEvents(fItems[i], type);
        }
    }

    Poller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList)
        : fItems()
        , fNumItems(0)
        , fOffsetMap()
    {
        try {
            int offset = 0;
            // calculate offsets and the total size of the poll item set
            for (std::string channel : channelList) {
                fOffsetMap[channel] = offset;
                offset += channelsMap.at(channel).size();
                fNumItems += channelsMap.at(channel).size();
            }

            fItems = new zmq_pollitem_t[fNumItems];

            int index = 0;
            for (std::string channel : channelList) {
                for (unsigned int i = 0; i < channelsMap.at(channel).size(); ++i) {
                    index = fOffsetMap[channel] + i;

                    fItems[index].socket = static_cast<const Socket*>(&(channelsMap.at(channel).at(i).GetSocket()))->GetSocket();
                    fItems[index].fd = 0;
                    fItems[index].revents = 0;

                    int type = 0;
                    size_t size = sizeof(type);
                    zmq_getsockopt(static_cast<const Socket*>(&(channelsMap.at(channel).at(i).GetSocket()))->GetSocket(), ZMQ_TYPE, &type, &size);

                    SetItemEvents(fItems[index], type);
                }
            }
        } catch (const std::out_of_range& oor) {
            LOG(error) << "At least one of the provided channel keys for poller initialization is invalid." << " Out of range error: " << oor.what();
            throw fair::mq::PollerError(fair::mq::tools::ToString("At least one of the provided channel keys for poller initialization is invalid. ", "Out of range error: ", oor.what()));
        }
    }

    Poller(const Poller&) = delete;
    Poller operator=(const Poller&) = delete;

    void SetItemEvents(zmq_pollitem_t& item, const int type)
    {
        if (type == ZMQ_REQ || type == ZMQ_REP || type == ZMQ_PAIR || type == ZMQ_DEALER || type == ZMQ_ROUTER) {
            item.events = ZMQ_POLLIN | ZMQ_POLLOUT;
        } else if (type == ZMQ_PUSH || type == ZMQ_PUB || type == ZMQ_XPUB) {
            item.events = ZMQ_POLLOUT;
        } else if (type == ZMQ_PULL || type == ZMQ_SUB || type == ZMQ_XSUB) {
            item.events = ZMQ_POLLIN;
        } else {
            LOG(error) << "invalid poller configuration, exiting.";
            throw fair::mq::PollerError("Invalid poller configuration, exiting.");
        }
    }

    void Poll(const int timeout) override
    {
        while (true) {
            if (zmq_poll(fItems, fNumItems, timeout) < 0) {
                if (errno == ETERM) {
                    LOG(debug) << "polling exited, reason: " << zmq_strerror(errno);
                    return;
                } else if (errno == EINTR) {
                    LOG(debug) << "polling interrupted by system call";
                    continue;
                } else {
                    LOG(error) << "polling failed, reason: " << zmq_strerror(errno);
                    throw fair::mq::PollerError(fair::mq::tools::ToString("Polling failed, reason: ", zmq_strerror(errno)));
                }
            }
            break;
        }
    }

    bool CheckInput(const int index) override
    {
        if (fItems[index].revents & ZMQ_POLLIN) {
            return true;
        }

        return false;
    }

    bool CheckOutput(const int index) override
    {
        if (fItems[index].revents & ZMQ_POLLOUT) {
            return true;
        }

        return false;
    }

    bool CheckInput(const std::string& channelKey, const int index) override
    {
        try {
            if (fItems[fOffsetMap.at(channelKey) + index].revents & ZMQ_POLLIN) {
                return true;
            }

            return false;
        } catch (const std::out_of_range& oor) {
            LOG(error) << "invalid channel key: '" << channelKey << "'";
            LOG(error) << "out of range error: " << oor.what();
            throw fair::mq::PollerError(fair::mq::tools::ToString("Invalid channel key '", channelKey, "'. Out of range error: ", oor.what()));
        }
    }

    bool CheckOutput(const std::string& channelKey, const int index) override
    {
        try {
            if (fItems[fOffsetMap.at(channelKey) + index].revents & ZMQ_POLLOUT) {
                return true;
            }

            return false;
        } catch (const std::out_of_range& oor) {
            LOG(error) << "invalid channel key: '" << channelKey << "'";
            LOG(error) << "out of range error: " << oor.what();
            throw fair::mq::PollerError(fair::mq::tools::ToString("Invalid channel key '", channelKey, "'. Out of range error: ", oor.what()));
        }
    }

    ~Poller() override { delete[] fItems; }

  private:
    zmq_pollitem_t* fItems;
    int fNumItems;

    std::unordered_map<std::string, int> fOffsetMap;
};

}   // namespace shmem
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SHMEM_POLLER_H_ */

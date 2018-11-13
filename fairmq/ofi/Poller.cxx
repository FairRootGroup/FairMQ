/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/ofi/Poller.h>
#include <fairmq/ofi/Socket.h>
#include <fairmq/Tools.h>
#include <FairMQLogger.h>

#include <zmq.h>

namespace fair
{
namespace mq
{
namespace ofi
{

using namespace std;

Poller::Poller(const vector<FairMQChannel>& channels)
{
    fNumItems = channels.size();
    fItems = new zmq_pollitem_t[fNumItems];

    for (int i = 0; i < fNumItems; ++i) {
        fItems[i].socket = static_cast<Socket*>(&(channels.at(i).GetSocket()))->GetSocket();
        fItems[i].fd = 0;
        fItems[i].revents = 0;

        int type = 0;
        size_t size = sizeof(type);
        zmq_getsockopt(static_cast<Socket*>(&(channels.at(i).GetSocket()))->GetSocket(), ZMQ_TYPE, &type, &size);

        SetItemEvents(fItems[i], type);
    }
}

Poller::Poller(const vector<const FairMQChannel*>& channels)
{
    fNumItems = channels.size();
    fItems = new zmq_pollitem_t[fNumItems];

    for (int i = 0; i < fNumItems; ++i) {
        fItems[i].socket = static_cast<Socket*>(&(channels.at(i)->GetSocket()))->GetSocket();
        fItems[i].fd = 0;
        fItems[i].revents = 0;

        int type = 0;
        size_t size = sizeof(type);
        zmq_getsockopt(static_cast<Socket*>(&(channels.at(i)->GetSocket()))->GetSocket(), ZMQ_TYPE, &type, &size);

        SetItemEvents(fItems[i], type);
    }
}

Poller::Poller(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList)
{
    try {
        int offset = 0;
        // calculate offsets and the total size of the poll item set
        for (string channel : channelList) {
            fOffsetMap[channel] = offset;
            offset += channelsMap.at(channel).size();
            fNumItems += channelsMap.at(channel).size();
        }

        fItems = new zmq_pollitem_t[fNumItems];

        int index = 0;
        for (string channel : channelList) {
            for (unsigned int i = 0; i < channelsMap.at(channel).size(); ++i) {
                index = fOffsetMap[channel] + i;

                fItems[index].socket = static_cast<Socket*>(&(channelsMap.at(channel).at(i).GetSocket()))->GetSocket();
                fItems[index].fd = 0;
                fItems[index].revents = 0;

                int type = 0;
                size_t size = sizeof(type);
                zmq_getsockopt(static_cast<Socket*>(&(channelsMap.at(channel).at(i).GetSocket()))->GetSocket(), ZMQ_TYPE, &type, &size);

                SetItemEvents(fItems[index], type);
            }
        }
    }
    catch (const std::out_of_range& oor) {
        throw PollerError{tools::ToString("At least one of the provided channel keys for poller initialization is invalid. ",
            "Out of range error: ", oor.what())};
    }
}

auto Poller::SetItemEvents(zmq_pollitem_t& item, const int type) -> void
{
    if (type == ZMQ_PAIR) {
        item.events = ZMQ_POLLIN|ZMQ_POLLOUT;
    } else {
        throw PollerError{"Invalid poller configuration."};
    }
}

auto Poller::Poll(const int timeout) -> void
{
    if (zmq_poll(fItems, fNumItems, timeout) < 0) {
        if (errno == ETERM) {
            LOG(debug) << "polling exited, reason: " << zmq_strerror(errno);
        } else {
            throw PollerError{tools::ToString("Polling failed, reason: ", zmq_strerror(errno))};
        }
    }
}

auto Poller::CheckInput(const int index) -> bool
{
    return fItems[index].revents & ZMQ_POLLIN;
}

auto Poller::CheckOutput(const int index) -> bool
{
    return fItems[index].revents & ZMQ_POLLOUT;
}

auto Poller::CheckInput(const string& channelKey, const int index) -> bool
{
    try {
        return fItems[fOffsetMap.at(channelKey) + index].revents & ZMQ_POLLIN;
    } catch (const std::out_of_range& oor) {
        throw PollerError{tools::ToString(
            "Invalid channel key: '", channelKey, "', ",
            "Out of range error: ", oor.what()
        )};
    }
}

auto Poller::CheckOutput(const string& channelKey, const int index) -> bool
{
    try {
        return fItems[fOffsetMap.at(channelKey) + index].revents & ZMQ_POLLOUT;
    } catch (const std::out_of_range& oor) {
        throw PollerError{tools::ToString(
            "Invalid channel key: '", channelKey, "', ",
            "Out of range error: ", oor.what()
        )};
    }
}

Poller::~Poller()
{
    delete[] fItems;
}

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */

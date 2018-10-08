/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQPollerSHM.cxx
 *
 * @since 2014-01-23
 * @author A. Rybalchenko
 */

#include "FairMQPollerSHM.h"
#include "FairMQLogger.h"

#include <zmq.h>

using namespace std;

FairMQPollerSHM::FairMQPollerSHM(const vector<FairMQChannel>& channels)
    : fItems()
    , fNumItems(0)
    , fOffsetMap()
{
    fNumItems = channels.size();
    fItems = new zmq_pollitem_t[fNumItems];

    for (int i = 0; i < fNumItems; ++i)
    {
        fItems[i].socket = channels.at(i).GetSocket().GetSocket();
        fItems[i].fd = 0;
        fItems[i].revents = 0;

        int type = 0;
        size_t size = sizeof(type);
        zmq_getsockopt(channels.at(i).GetSocket().GetSocket(), ZMQ_TYPE, &type, &size);

        SetItemEvents(fItems[i], type);
    }
}

FairMQPollerSHM::FairMQPollerSHM(const vector<const FairMQChannel*>& channels)
    : fItems()
    , fNumItems(0)
    , fOffsetMap()
{
    fNumItems = channels.size();
    fItems = new zmq_pollitem_t[fNumItems];

    for (int i = 0; i < fNumItems; ++i)
    {
        fItems[i].socket = channels.at(i)->GetSocket().GetSocket();
        fItems[i].fd = 0;
        fItems[i].revents = 0;

        int type = 0;
        size_t size = sizeof(type);
        zmq_getsockopt(channels.at(i)->GetSocket().GetSocket(), ZMQ_TYPE, &type, &size);

        SetItemEvents(fItems[i], type);
    }
}

FairMQPollerSHM::FairMQPollerSHM(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList)
    : fItems()
    , fNumItems(0)
    , fOffsetMap()
{
    try
    {
        int offset = 0;
        // calculate offsets and the total size of the poll item set
        for (string channel : channelList)
        {
            fOffsetMap[channel] = offset;
            offset += channelsMap.at(channel).size();
            fNumItems += channelsMap.at(channel).size();
        }

        fItems = new zmq_pollitem_t[fNumItems];

        int index = 0;
        for (string channel : channelList)
        {
            for (unsigned int i = 0; i < channelsMap.at(channel).size(); ++i)
            {
                index = fOffsetMap[channel] + i;

                fItems[index].socket = channelsMap.at(channel).at(i).GetSocket().GetSocket();
                fItems[index].fd = 0;
                fItems[index].revents = 0;

                int type = 0;
                size_t size = sizeof(type);
                zmq_getsockopt(channelsMap.at(channel).at(i).GetSocket().GetSocket(), ZMQ_TYPE, &type, &size);

                SetItemEvents(fItems[index], type);
            }
        }
    }
    catch (const std::out_of_range& oor)
    {
        LOG(error) << "at least one of the provided channel keys for poller initialization is invalid";
        LOG(error) << "out of range error: " << oor.what() << '\n';
        exit(EXIT_FAILURE);
    }
}

FairMQPollerSHM::FairMQPollerSHM(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket)
    : fItems()
    , fNumItems(2)
    , fOffsetMap()
{
    fItems = new zmq_pollitem_t[fNumItems];

    fItems[0].socket = cmdSocket.GetSocket();
    fItems[0].fd = 0;
    fItems[0].events = ZMQ_POLLIN;
    fItems[0].revents = 0;

    fItems[1].socket = dataSocket.GetSocket();
    fItems[1].fd = 0;
    fItems[1].revents = 0;

    int type = 0;
    size_t size = sizeof(type);
    zmq_getsockopt(dataSocket.GetSocket(), ZMQ_TYPE, &type, &size);

    SetItemEvents(fItems[1], type);
}

void FairMQPollerSHM::SetItemEvents(zmq_pollitem_t& item, const int type)
{
    if (type == ZMQ_REQ || type == ZMQ_REP || type == ZMQ_PAIR || type == ZMQ_DEALER || type == ZMQ_ROUTER)
    {
        item.events = ZMQ_POLLIN|ZMQ_POLLOUT;
    }
    else if (type == ZMQ_PUSH || type == ZMQ_PUB || type == ZMQ_XPUB)
    {
        item.events = ZMQ_POLLOUT;
    }
    else if (type == ZMQ_PULL || type == ZMQ_SUB || type == ZMQ_XSUB)
    {
        item.events = ZMQ_POLLIN;
    }
    else
    {
        LOG(error) << "invalid poller configuration, exiting.";
        exit(EXIT_FAILURE);
    }
}

void FairMQPollerSHM::Poll(const int timeout)
{
    if (zmq_poll(fItems, fNumItems, timeout) < 0)
    {
        if (errno == ETERM)
        {
            LOG(debug) << "polling exited, reason: " << zmq_strerror(errno);
        }
        else
        {
            LOG(error) << "polling failed, reason: " << zmq_strerror(errno);
            throw std::runtime_error("polling failed");
        }
    }
}

bool FairMQPollerSHM::CheckInput(const int index)
{
    if (fItems[index].revents & ZMQ_POLLIN)
    {
        return true;
    }

    return false;
}

bool FairMQPollerSHM::CheckOutput(const int index)
{
    if (fItems[index].revents & ZMQ_POLLOUT)
    {
        return true;
    }

    return false;
}

bool FairMQPollerSHM::CheckInput(const string& channelKey, const int index)
{
    try
    {
        if (fItems[fOffsetMap.at(channelKey) + index].revents & ZMQ_POLLIN)
        {
            return true;
        }

        return false;
    }
    catch (const std::out_of_range& oor)
    {
        LOG(error) << "invalid channel key: \"" << channelKey << "\"";
        LOG(error) << "out of range error: " << oor.what() << '\n';
        exit(EXIT_FAILURE);
    }
}

bool FairMQPollerSHM::CheckOutput(const string& channelKey, const int index)
{
    try
    {
        if (fItems[fOffsetMap.at(channelKey) + index].revents & ZMQ_POLLOUT)
        {
            return true;
        }

        return false;
    }
    catch (const std::out_of_range& oor)
    {
        LOG(error) << "Invalid channel key: \"" << channelKey << "\"";
        LOG(error) << "out of range error: " << oor.what() << '\n';
        exit(EXIT_FAILURE);
    }
}

FairMQPollerSHM::~FairMQPollerSHM()
{
    delete[] fItems;
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQPollerNN.cxx
 *
 * @since 2014-01-23
 * @author A. Rybalchenko
 */

#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pair.h>

#include "FairMQPollerNN.h"
#include "FairMQLogger.h"

using namespace std;

FairMQPollerNN::FairMQPollerNN(const vector<FairMQChannel>& channels)
    : fItems()
    , fNumItems(0)
    , fOffsetMap()
{
    fNumItems = channels.size();
    fItems = new nn_pollfd[fNumItems];

    for (int i = 0; i < fNumItems; ++i)
    {
        fItems[i].fd = channels.at(i).GetSocket().GetSocket(1);

        int type = 0;
        size_t sz = sizeof(type);
        nn_getsockopt(channels.at(i).GetSocket().GetSocket(1), NN_SOL_SOCKET, NN_PROTOCOL, &type, &sz);

        SetItemEvents(fItems[i], type);
    }
}

FairMQPollerNN::FairMQPollerNN(const vector<const FairMQChannel*>& channels)
    : fItems()
    , fNumItems(0)
    , fOffsetMap()
{
    fNumItems = channels.size();
    fItems = new nn_pollfd[fNumItems];

    for (int i = 0; i < fNumItems; ++i)
    {
        fItems[i].fd = channels.at(i)->GetSocket().GetSocket(1);

        int type = 0;
        size_t sz = sizeof(type);
        nn_getsockopt(channels.at(i)->GetSocket().GetSocket(1), NN_SOL_SOCKET, NN_PROTOCOL, &type, &sz);

        SetItemEvents(fItems[i], type);
    }
}

FairMQPollerNN::FairMQPollerNN(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList)
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

        fItems = new nn_pollfd[fNumItems];

        int index = 0;
        for (string channel : channelList)
        {
            for (unsigned int i = 0; i < channelsMap.at(channel).size(); ++i)
            {
                index = fOffsetMap[channel] + i;
                fItems[index].fd = channelsMap.at(channel).at(i).GetSocket().GetSocket(1);

                int type = 0;
                size_t sz = sizeof(type);
                nn_getsockopt(channelsMap.at(channel).at(i).GetSocket().GetSocket(1), NN_SOL_SOCKET, NN_PROTOCOL, &type, &sz);

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

FairMQPollerNN::FairMQPollerNN(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket)
    : fItems()
    , fNumItems(2)
    , fOffsetMap()
{
    fItems = new nn_pollfd[fNumItems];

    fItems[0].fd = cmdSocket.GetSocket(1);
    fItems[0].events = NN_POLLIN;
    fItems[0].revents = 0;

    fItems[1].fd = dataSocket.GetSocket(1);
    fItems[1].revents = 0;

    int type = 0;
    size_t sz = sizeof(type);
    nn_getsockopt(dataSocket.GetSocket(1), NN_SOL_SOCKET, NN_PROTOCOL, &type, &sz);

    SetItemEvents(fItems[1], type);
}

void FairMQPollerNN::SetItemEvents(nn_pollfd& item, const int type)
{
    if (type == NN_REQ || type == NN_REP || type == NN_PAIR)
    {
        item.events = NN_POLLIN|NN_POLLOUT;
    }
    else if (type == NN_PUSH || type == NN_PUB)
    {
        item.events = NN_POLLOUT;
    }
    else if (type == NN_PULL || type == NN_SUB)
    {
        item.events = NN_POLLIN;
    }
    else
    {
        LOG(error) << "invalid poller configuration, exiting.";
        exit(EXIT_FAILURE);
    }
}

void FairMQPollerNN::Poll(const int timeout)
{
    if (nn_poll(fItems, fNumItems, timeout) < 0)
    {
        if (errno == ETERM)
        {
            LOG(debug) << "polling exited, reason: " << nn_strerror(errno);
        }
        else
        {
            LOG(error) << "polling failed, reason: " << nn_strerror(errno);
            throw std::runtime_error("polling failed");
        }
    }
}

bool FairMQPollerNN::CheckInput(const int index)
{
    if (fItems[index].revents & (NN_POLLIN | NN_POLLOUT))
    {
        return true;
    }

    return false;
}

bool FairMQPollerNN::CheckOutput(const int index)
{
    if (fItems[index].revents & NN_POLLOUT)
    {
        return true;
    }

    return false;
}

bool FairMQPollerNN::CheckInput(const string& channelKey, const int index)
{
    try
    {
        if (fItems[fOffsetMap.at(channelKey) + index].revents & (NN_POLLIN | NN_POLLOUT))
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

bool FairMQPollerNN::CheckOutput(const string& channelKey, const int index)
{
    try
    {
        if (fItems[fOffsetMap.at(channelKey) + index].revents & NN_POLLOUT)
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

FairMQPollerNN::~FairMQPollerNN()
{
    delete[] fItems;
}

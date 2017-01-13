/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
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
    : items()
    , fNumItems(0)
    , fOffsetMap()
{
    fNumItems = channels.size();
    items = new nn_pollfd[fNumItems];

    for (int i = 0; i < fNumItems; ++i)
    {
        items[i].fd = channels.at(i).fSocket->GetSocket(1);

        int type = 0;
        size_t sz = sizeof(type);
        nn_getsockopt(channels.at(i).fSocket->GetSocket(1), NN_SOL_SOCKET, NN_PROTOCOL, &type, &sz);

        if (type == NN_REQ || type == NN_REP || type == NN_PAIR)
        {
            items[i].events = NN_POLLIN|NN_POLLOUT;
        }
        else if (type == NN_PUSH || type == NN_PUB)
        {
            items[i].events = NN_POLLOUT;
        }
        else if (type == NN_PULL || type == NN_SUB)
        {
            items[i].events = NN_POLLIN;
        }
        else
        {
            LOG(ERROR) << "nanomsg: invalid poller configuration, exiting.";
            exit(EXIT_FAILURE);
        }
    }
}

FairMQPollerNN::FairMQPollerNN(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList)
    : items()
    , fNumItems(0)
    , fOffsetMap()
{
    int offset = 0;

    try
    {
        // calculate offsets and the total size of the poll item set
        for (string channel : channelList)
        {
            fOffsetMap[channel] = offset;
            offset += channelsMap.at(channel).size();
            fNumItems += channelsMap.at(channel).size();
        }

        items = new nn_pollfd[fNumItems];

        int index = 0;
        for (string channel : channelList)
        {
            for (unsigned int i = 0; i < channelsMap.at(channel).size(); ++i)
            {
                index = fOffsetMap[channel] + i;
                items[index].fd = channelsMap.at(channel).at(i).fSocket->GetSocket(1);

                int type = 0;
                size_t sz = sizeof(type);
                nn_getsockopt(channelsMap.at(channel).at(i).fSocket->GetSocket(1), NN_SOL_SOCKET, NN_PROTOCOL, &type, &sz);

                if (type == NN_REQ || type == NN_REP || type == NN_PAIR)
                {
                    items[index].events = NN_POLLIN|NN_POLLOUT;
                }
                else if (type == NN_PUSH || type == NN_PUB)
                {
                    items[index].events = NN_POLLOUT;
                }
                else if (type == NN_PULL || type == NN_SUB)
                {
                    items[index].events = NN_POLLIN;
                }
                else
                {
                    LOG(ERROR) << "nanomsg: invalid poller configuration, exiting.";
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    catch (const std::out_of_range& oor)
    {
        LOG(ERROR) << "nanomsg: at least one of the provided channel keys for poller initialization is invalid";
        LOG(ERROR) << "nanomsg: out of range error: " << oor.what() << '\n';
        exit(EXIT_FAILURE);
    }
}

FairMQPollerNN::FairMQPollerNN(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket)
    : items()
    , fNumItems(2)
    , fOffsetMap()
{
    items = new nn_pollfd[fNumItems];

    items[0].fd = cmdSocket.GetSocket(1);
    items[0].events = NN_POLLIN;
    items[0].revents = 0;

    items[1].fd = dataSocket.GetSocket(1);
    items[1].revents = 0;

    int type = 0;
    size_t sz = sizeof(type);
    nn_getsockopt(dataSocket.GetSocket(1), NN_SOL_SOCKET, NN_PROTOCOL, &type, &sz);

    if (type == NN_REQ || type == NN_REP || type == NN_PAIR)
    {
        items[1].events = NN_POLLIN|NN_POLLOUT;
    }
    else if (type == NN_PUSH || type == NN_PUB)
    {
        items[1].events = NN_POLLOUT;
    }
    else if (type == NN_PULL || type == NN_SUB)
    {
        items[1].events = NN_POLLIN;
    }
    else
    {
        LOG(ERROR) << "nanomsg: invalid poller configuration, exiting.";
        exit(EXIT_FAILURE);
    }
}

void FairMQPollerNN::Poll(const int timeout)
{
    if (nn_poll(items, fNumItems, timeout) < 0)
    {
        if (errno == ETERM)
        {
            LOG(DEBUG) << "nanomsg: polling exited, reason: " << nn_strerror(errno);
        }
        else
        {
            LOG(ERROR) << "nanomsg: polling failed, reason: " << nn_strerror(errno);
            throw std::runtime_error("nanomsg: polling failed");
        }
    }
}

bool FairMQPollerNN::CheckInput(const int index)
{
    if (items[index].revents & (NN_POLLIN | NN_POLLOUT))
    {
        return true;
    }

    return false;
}

bool FairMQPollerNN::CheckOutput(const int index)
{
    if (items[index].revents & NN_POLLOUT)
    {
        return true;
    }

    return false;
}

bool FairMQPollerNN::CheckInput(const string channelKey, const int index)
{
    try
    {
        if (items[fOffsetMap.at(channelKey) + index].revents & (NN_POLLIN | NN_POLLOUT))
        {
            return true;
        }

        return false;
    }
    catch (const std::out_of_range& oor)
    {
        LOG(ERROR) << "nanomsg: invalid channel key: \"" << channelKey << "\"";
        LOG(ERROR) << "nanomsg: out of range error: " << oor.what() << '\n';
        exit(EXIT_FAILURE);
    }
}

bool FairMQPollerNN::CheckOutput(const string channelKey, const int index)
{
    try
    {
        if (items[fOffsetMap.at(channelKey) + index].revents & NN_POLLOUT)
        {
            return true;
        }

        return false;
    }
    catch (const std::out_of_range& oor)
    {
        LOG(ERROR) << "nanomsg: invalid channel key: \"" << channelKey << "\"";
        LOG(ERROR) << "nanomsg: out of range error: " << oor.what() << '\n';
        exit(EXIT_FAILURE);
    }
}

FairMQPollerNN::~FairMQPollerNN()
{
    if (items != NULL)
    {
        delete[] items;
    }
}

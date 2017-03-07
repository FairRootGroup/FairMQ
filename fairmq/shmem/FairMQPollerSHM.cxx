/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQPollerSHM.cxx
 *
 * @since 2014-01-23
 * @author A. Rybalchenko
 */

#include <zmq.h>

#include "FairMQPollerSHM.h"
#include "FairMQLogger.h"

using namespace std;

FairMQPollerSHM::FairMQPollerSHM(const vector<FairMQChannel>& channels)
    : items()
    , fNumItems(0)
    , fOffsetMap()
{
    fNumItems = channels.size();
    items = new zmq_pollitem_t[fNumItems];

    for (int i = 0; i < fNumItems; ++i)
    {
        items[i].socket = channels.at(i).GetSocket().GetSocket();
        items[i].fd = 0;
        items[i].revents = 0;

        int type = 0;
        size_t size = sizeof(type);
        zmq_getsockopt(channels.at(i).GetSocket().GetSocket(), ZMQ_TYPE, &type, &size);

        if (type == ZMQ_REQ || type == ZMQ_REP || type == ZMQ_PAIR || type == ZMQ_DEALER || type == ZMQ_ROUTER)
        {
            items[i].events = ZMQ_POLLIN|ZMQ_POLLOUT;
        }
        else if (type == ZMQ_PUSH || type == ZMQ_PUB || type == ZMQ_XPUB)
        {
            items[i].events = ZMQ_POLLOUT;
        }
        else if (type == ZMQ_PULL || type == ZMQ_SUB || type == ZMQ_XSUB)
        {
            items[i].events = ZMQ_POLLIN;
        }
        else
        {
            LOG(ERROR) << "shmem: invalid poller configuration, exiting.";
            exit(EXIT_FAILURE);
        }
    }
}

FairMQPollerSHM::FairMQPollerSHM(const unordered_map<string, vector<FairMQChannel>>& channelsMap, const vector<string>& channelList)
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

        items = new zmq_pollitem_t[fNumItems];

        int index = 0;
        for (string channel : channelList)
        {
            for (unsigned int i = 0; i < channelsMap.at(channel).size(); ++i)
            {
                index = fOffsetMap[channel] + i;

                items[index].socket = channelsMap.at(channel).at(i).GetSocket().GetSocket();
                items[index].fd = 0;
                items[index].revents = 0;

                int type = 0;
                size_t size = sizeof(type);
                zmq_getsockopt(channelsMap.at(channel).at(i).GetSocket().GetSocket(), ZMQ_TYPE, &type, &size);

                if (type == ZMQ_REQ || type == ZMQ_REP || type == ZMQ_PAIR || type == ZMQ_DEALER || type == ZMQ_ROUTER)
                {
                    items[index].events = ZMQ_POLLIN|ZMQ_POLLOUT;
                }
                else if (type == ZMQ_PUSH || type == ZMQ_PUB || type == ZMQ_XPUB)
                {
                    items[index].events = ZMQ_POLLOUT;
                }
                else if (type == ZMQ_PULL || type == ZMQ_SUB || type == ZMQ_XSUB)
                {
                    items[index].events = ZMQ_POLLIN;
                }
                else
                {
                    LOG(ERROR) << "shmem: invalid poller configuration, exiting.";
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    catch (const std::out_of_range& oor)
    {
        LOG(ERROR) << "shmem: at least one of the provided channel keys for poller initialization is invalid";
        LOG(ERROR) << "shmem: out of range error: " << oor.what() << '\n';
        exit(EXIT_FAILURE);
    }
}

FairMQPollerSHM::FairMQPollerSHM(const FairMQSocket& cmdSocket, const FairMQSocket& dataSocket)
    : items()
    , fNumItems(2)
    , fOffsetMap()
{
    items = new zmq_pollitem_t[fNumItems];

    items[0].socket = cmdSocket.GetSocket();
    items[0].fd = 0;
    items[0].events = ZMQ_POLLIN;
    items[0].revents = 0;

    items[1].socket = dataSocket.GetSocket();
    items[1].fd = 0;
    items[1].revents = 0;

    int type = 0;
    size_t size = sizeof(type);
    zmq_getsockopt(dataSocket.GetSocket(), ZMQ_TYPE, &type, &size);

    if (type == ZMQ_REQ || type == ZMQ_REP || type == ZMQ_PAIR || type == ZMQ_DEALER || type == ZMQ_ROUTER)
    {
        items[1].events = ZMQ_POLLIN|ZMQ_POLLOUT;
    }
    else if (type == ZMQ_PUSH || type == ZMQ_PUB || type == ZMQ_XPUB)
    {
        items[1].events = ZMQ_POLLOUT;
    }
    else if (type == ZMQ_PULL || type == ZMQ_SUB || type == ZMQ_XSUB)
    {
        items[1].events = ZMQ_POLLIN;
    }
    else
    {
        LOG(ERROR) << "shmem: invalid poller configuration, exiting.";
        exit(EXIT_FAILURE);
    }
}

void FairMQPollerSHM::Poll(const int timeout)
{
    if (zmq_poll(items, fNumItems, timeout) < 0)
    {
        if (errno == ETERM)
        {
            LOG(DEBUG) << "shmem: polling exited, reason: " << zmq_strerror(errno);
        }
        else
        {
            LOG(ERROR) << "shmem: polling failed, reason: " << zmq_strerror(errno);
            throw std::runtime_error("shmem: polling failed");
        }
    }
}

bool FairMQPollerSHM::CheckInput(const int index)
{
    if (items[index].revents & ZMQ_POLLIN)
    {
        return true;
    }

    return false;
}

bool FairMQPollerSHM::CheckOutput(const int index)
{
    if (items[index].revents & ZMQ_POLLOUT)
    {
        return true;
    }

    return false;
}

bool FairMQPollerSHM::CheckInput(const string channelKey, const int index)
{
    try
    {
        if (items[fOffsetMap.at(channelKey) + index].revents & ZMQ_POLLIN)
        {
            return true;
        }

        return false;
    }
    catch (const std::out_of_range& oor)
    {
        LOG(ERROR) << "shmem: invalid channel key: \"" << channelKey << "\"";
        LOG(ERROR) << "shmem: out of range error: " << oor.what() << '\n';
        exit(EXIT_FAILURE);
    }
}

bool FairMQPollerSHM::CheckOutput(const string channelKey, const int index)
{
    try
    {
        if (items[fOffsetMap.at(channelKey) + index].revents & ZMQ_POLLOUT)
        {
            return true;
        }

        return false;
    }
    catch (const std::out_of_range& oor)
    {
        LOG(ERROR) << "shmem: Invalid channel key: \"" << channelKey << "\"";
        LOG(ERROR) << "shmem: out of range error: " << oor.what() << '\n';
        exit(EXIT_FAILURE);
    }
}

FairMQPollerSHM::~FairMQPollerSHM()
{
    delete[] items;
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQPollerZMQ.cxx
 *
 * @since 2014-01-23
 * @author A. Rybalchenko
 */

#include <zmq.h>

#include "FairMQPollerZMQ.h"
#include "FairMQLogger.h"

using namespace std;

FairMQPollerZMQ::FairMQPollerZMQ(const vector<FairMQChannel>& channels)
    : items()
    , fNumItems()
{
    fNumItems = channels.size();
    items = new zmq_pollitem_t[fNumItems];

    for (int i = 0; i < fNumItems; ++i)
    {
        items[i].socket = channels.at(i).fSocket->GetSocket();
        items[i].fd = 0;
        items[i].events = ZMQ_POLLIN;
        items[i].revents = 0;
    }
}

void FairMQPollerZMQ::Poll(int timeout)
{
    if (zmq_poll(items, fNumItems, timeout) < 0)
    {
        LOG(ERROR) << "polling failed, reason: " << zmq_strerror(errno);
    }
}

bool FairMQPollerZMQ::CheckInput(int index)
{
    if (items[index].revents & ZMQ_POLLIN)
    {
        return true;
    }

    return false;
}

bool FairMQPollerZMQ::CheckOutput(int index)
{
    if (items[index].revents & ZMQ_POLLOUT)
    {
        return true;
    }

    return false;
}

FairMQPollerZMQ::~FairMQPollerZMQ()
{
    if (items != NULL)
    {
        delete[] items;
    }
}

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

#include "FairMQPollerNN.h"

FairMQPollerNN::FairMQPollerNN(const vector<FairMQSocket*>& inputs)
{
    fNumItems = inputs.size();
    items = new nn_pollfd[fNumItems];

    for (int i = 0; i < fNumItems; i++)
    {
        items[i].fd = inputs.at(i)->GetSocket(1);
        items[i].events = NN_POLLIN;
    }
}

void FairMQPollerNN::Poll(int timeout)
{
    nn_poll(items, fNumItems, timeout);
}

bool FairMQPollerNN::CheckInput(int index)
{
    if (items[index].revents & NN_POLLIN)
        return true;

    return false;
}

FairMQPollerNN::~FairMQPollerNN()
{
    if (items != NULL)
        delete[] items;
}

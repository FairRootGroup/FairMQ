/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSplitter.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQLogger.h"
#include "FairMQSplitter.h"

FairMQSplitter::FairMQSplitter()
{
}

FairMQSplitter::~FairMQSplitter()
{
}

void FairMQSplitter::Run()
{
    int direction = 0;
    int numOutputs = fChannels.at("data-out").size();

    // store the channel references to avoid traversing the map on every loop iteration
    const FairMQChannel& dataInChannel = fChannels.at("data-in").at(0);
    FairMQChannel* dataOutChannels[fChannels.at("data-out").size()];
    for (int i = 0; i < numOutputs; ++i)
    {
        dataOutChannels[i] = &(fChannels.at("data-out").at(i));
    }

    while (CheckCurrentState(RUNNING))
    {
        std::unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());

        if (dataInChannel.Receive(msg) > 0)
        {
            dataOutChannels[direction]->Send(msg);
            ++direction;
            if (direction >= numOutputs)
            {
                direction = 0;
            }
        }
    }
}

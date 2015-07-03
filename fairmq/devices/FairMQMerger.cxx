/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQMerger.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQLogger.h"
#include "FairMQMerger.h"
#include "FairMQPoller.h"

FairMQMerger::FairMQMerger()
{
}

FairMQMerger::~FairMQMerger()
{
}

void FairMQMerger::Run()
{
    FairMQPoller* poller = fTransportFactory->CreatePoller(fChannels.at("data-in"));

    // store the channel references to avoid traversing the map on every loop iteration
    const FairMQChannel& dataOutChannel = fChannels.at("data-out").at(0);
    FairMQChannel* dataInChannels[fChannels.at("data-in").size()];
    for (int i = 0; i < fChannels.at("data-in").size(); ++i)
    {
        dataInChannels[i] = &(fChannels.at("data-in").at(i));
    }

    while (CheckCurrentState(RUNNING))
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();

        poller->Poll(100);

        for (int i = 0; i < fChannels.at("data-in").size(); ++i)
        {
            if (poller->CheckInput(i))
            {
                if (dataInChannels[i]->Receive(msg) > 0)
                {
                    if (dataOutChannel.Send(msg) < 0)
                    {
                        LOG(DEBUG) << "Blocking send interrupted by a command";
                        break;
                    }
                }
                else
                {
                    LOG(DEBUG) << "Blocking receive interrupted by a command";
                    break;
                }
            }
        }

        delete msg;
    }

    delete poller;
}

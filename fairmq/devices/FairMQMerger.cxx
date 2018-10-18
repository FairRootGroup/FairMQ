/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQMerger.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include "FairMQMerger.h"
#include "../FairMQLogger.h"
#include "../FairMQPoller.h"
#include "../options/FairMQProgOptions.h"

using namespace std;

FairMQMerger::FairMQMerger()
    : fMultipart(true)
    , fInChannelName("data-in")
    , fOutChannelName("data-out")
{
}

void FairMQMerger::RegisterChannelEndpoints()
{
    RegisterChannelEndpoint(fInChannelName, 1, 10000);
    RegisterChannelEndpoint(fOutChannelName, 1, 1);

    PrintRegisteredChannels();
}

FairMQMerger::~FairMQMerger()
{
}

void FairMQMerger::InitTask()
{
    fMultipart = fConfig->GetValue<bool>("multipart");
    fInChannelName = fConfig->GetValue<string>("in-channel");
    fOutChannelName = fConfig->GetValue<string>("out-channel");
}

void FairMQMerger::Run()
{
    int numInputs = fChannels.at(fInChannelName).size();

    vector<const FairMQChannel*> chans;

    for (auto& chan : fChannels.at(fInChannelName))
    {
        chans.push_back(&chan);
    }

    FairMQPollerPtr poller(NewPoller(chans));

    if (fMultipart)
    {
        while (CheckCurrentState(RUNNING))
        {
            poller->Poll(100);

            // Loop over the data input channels.
            for (int i = 0; i < numInputs; ++i)
            {
                // Check if the channel has data ready to be received.
                if (poller->CheckInput(i))
                {
                    FairMQParts payload;

                    if (Receive(payload, fInChannelName, i) >= 0)
                    {
                        if (Send(payload, fOutChannelName) < 0)
                        {
                            LOG(debug) << "Transfer interrupted";
                            break;
                        }
                    }
                    else
                    {
                        LOG(debug) << "Transfer interrupted";
                        break;
                    }
                }
            }
        }
    }
    else
    {
        while (CheckCurrentState(RUNNING))
        {
            poller->Poll(100);

            // Loop over the data input channels.
            for (int i = 0; i < numInputs; ++i)
            {
                // Check if the channel has data ready to be received.
                if (poller->CheckInput(i))
                {
                    FairMQMessagePtr payload(fTransportFactory->CreateMessage());

                    if (Receive(payload, fInChannelName, i) >= 0)
                    {
                        if (Send(payload, fOutChannelName) < 0)
                        {
                            LOG(debug) << "Transfer interrupted";
                            break;
                        }
                    }
                    else
                    {
                        LOG(debug) << "Transfer interrupted";
                        break;
                    }
                }
            }
        }
    }
}

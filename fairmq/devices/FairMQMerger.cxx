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

#include "FairMQLogger.h"
#include "FairMQMerger.h"
#include "FairMQPoller.h"
#include "FairMQProgOptions.h"

using namespace std;

FairMQMerger::FairMQMerger()
    : fMultipart(1)
    , fInChannelName()
    , fOutChannelName()
{
}

FairMQMerger::~FairMQMerger()
{
}

void FairMQMerger::InitTask()
{
    fMultipart = fConfig->GetValue<int>("multipart");
    fInChannelName = fConfig->GetValue<string>("in-channel");
    fOutChannelName = fConfig->GetValue<string>("out-channel");
}

void FairMQMerger::Run()
{
    int numInputs = fChannels.at(fInChannelName).size();

    std::unique_ptr<FairMQPoller> poller(fTransportFactory->CreatePoller(fChannels.at(fInChannelName)));

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
                            LOG(DEBUG) << "Transfer interrupted";
                            break;
                        }
                    }
                    else
                    {
                        LOG(DEBUG) << "Transfer interrupted";
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
                            LOG(DEBUG) << "Transfer interrupted";
                            break;
                        }
                    }
                    else
                    {
                        LOG(DEBUG) << "Transfer interrupted";
                        break;
                    }
                }
            }
        }
    }
}

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
    int numInputs = fChannels.at("data-in").size();

    std::unique_ptr<FairMQPoller> poller(fTransportFactory->CreatePoller(fChannels.at("data-in")));

    while (CheckCurrentState(RUNNING))
    {
        poller->Poll(100);

        // Loop over the data input channels.
        for (int i = 0; i < numInputs; ++i)
        {
            // Check if the channel has data ready to be received.
            if (poller->CheckInput(i))
            {
                FairMQParts parts;

                if (Receive(parts, "data-in", i) >= 0)
                {
                    if (Send(parts, "data-out") < 0)
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

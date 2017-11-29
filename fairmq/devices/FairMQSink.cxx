/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSink.cxx
 *
 * @since 2013-01-09
 * @author D. Klein, A. Rybalchenko
 */

#include "FairMQSink.h"

#include <chrono>

#include "../FairMQLogger.h"
#include "../options/FairMQProgOptions.h"

using namespace std;

FairMQSink::FairMQSink()
    : fNumMsgs(0)
    , fInChannelName()
{
}

void FairMQSink::InitTask()
{
    fNumMsgs = fConfig->GetValue<uint64_t>("num-msgs");
    fInChannelName = fConfig->GetValue<string>("in-channel");
}

void FairMQSink::Run()
{
    uint64_t numReceivedMsgs = 0;
    // store the channel reference to avoid traversing the map on every loop iteration
    FairMQChannel& dataInChannel = fChannels.at(fInChannelName).at(0);

    LOG(INFO) << "Starting the benchmark and expecting to receive " << fNumMsgs << " messages.";
    auto tStart = chrono::high_resolution_clock::now();

    while (CheckCurrentState(RUNNING))
    {
        FairMQMessagePtr msg(dataInChannel.Transport()->CreateMessage());

        if (dataInChannel.Receive(msg) >= 0)
        {
            if (fNumMsgs > 0)
            {
                if (numReceivedMsgs >= fNumMsgs)
                {
                    break;
                }
            }
            numReceivedMsgs++;
        }
    }

    auto tEnd = chrono::high_resolution_clock::now();

    LOG(INFO) << "Leaving RUNNING state. Received " << numReceivedMsgs << " messages in " << chrono::duration<double, milli>(tEnd - tStart).count() << "ms.";
}

FairMQSink::~FairMQSink()
{
}

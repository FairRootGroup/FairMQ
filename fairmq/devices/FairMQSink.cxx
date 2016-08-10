/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSink.cxx
 *
 * @since 2013-01-09
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/timer/timer.hpp>

#include "FairMQSink.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h"

using namespace std;

FairMQSink::FairMQSink()
    : fNumMsgs(0)
    , fInChannelName()
{
}

void FairMQSink::InitTask()
{
    fNumMsgs = fConfig->GetValue<int>("num-msgs");
    fInChannelName = fConfig->GetValue<string>("in-channel");
}

void FairMQSink::Run()
{
    int numReceivedMsgs = 0;
    // store the channel reference to avoid traversing the map on every loop iteration
    const FairMQChannel& dataInChannel = fChannels.at(fInChannelName).at(0);

    LOG(INFO) << "Starting the benchmark and expecting to receive " << fNumMsgs << " messages.";
    boost::timer::auto_cpu_timer timer;

    while (CheckCurrentState(RUNNING))
    {
        std::unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());

        if (dataInChannel.Receive(msg) >= 0)
        {
            if (fNumMsgs > 0)
            {
                numReceivedMsgs++;
                if (numReceivedMsgs >= fNumMsgs)
                {
                    break;
                }
            }
        }
    }

    LOG(INFO) << "Received " << numReceivedMsgs << " messages, leaving RUNNING state.";
    LOG(INFO) << "Receiving time: ";
}

FairMQSink::~FairMQSink()
{
}

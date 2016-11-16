/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQBenchmarkSampler.cpp
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <vector>
#include <chrono>
#include <thread>

#include "FairMQBenchmarkSampler.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h"

using namespace std;

FairMQBenchmarkSampler::FairMQBenchmarkSampler()
    : fMsgSize(10000)
    , fNumMsgs(0)
    , fMsgCounter(0)
    , fMsgRate(1)
    , fOutChannelName()
{
}

FairMQBenchmarkSampler::~FairMQBenchmarkSampler()
{
}

void FairMQBenchmarkSampler::InitTask()
{
    fMsgSize = fConfig->GetValue<int>("msg-size");
    fNumMsgs = fConfig->GetValue<int>("num-msgs");
    fMsgRate = fConfig->GetValue<int>("msg-rate");
    fOutChannelName = fConfig->GetValue<string>("out-channel");
}

void FairMQBenchmarkSampler::Run()
{
    // std::thread resetMsgCounter(&FairMQBenchmarkSampler::ResetMsgCounter, this);

    int numSentMsgs = 0;

    FairMQMessagePtr baseMsg(fTransportFactory->CreateMessage(fMsgSize));

    // store the channel reference to avoid traversing the map on every loop iteration
    const FairMQChannel& dataOutChannel = fChannels.at(fOutChannelName).at(0);

    LOG(INFO) << "Starting the benchmark with message size of " << fMsgSize << " and number of messages " << fNumMsgs << ".";
    auto tStart = chrono::high_resolution_clock::now();

    while (CheckCurrentState(RUNNING))
    {
        FairMQMessagePtr msg(fTransportFactory->CreateMessage());
        msg->Copy(baseMsg);

        if (dataOutChannel.Send(msg) >= 0)
        {
            if (fNumMsgs > 0)
            {
                if (++numSentMsgs >= fNumMsgs)
                {
                    break;
                }
            }
        }

        // --fMsgCounter;

        // while (fMsgCounter == 0) {
        //   this_thread::sleep_for(chrono::milliseconds(1));
        // }
    }

    auto tEnd = chrono::high_resolution_clock::now();

    LOG(INFO) << "Sent " << numSentMsgs << " messages, leaving RUNNING state.";
    LOG(INFO) << "Sending time: " << chrono::duration<double, milli>(tEnd - tStart).count() << " ms";

    // resetMsgCounter.join();
}

void FairMQBenchmarkSampler::ResetMsgCounter()
{
    while (CheckCurrentState(RUNNING))
    {
        fMsgCounter = fMsgRate / 100;
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}

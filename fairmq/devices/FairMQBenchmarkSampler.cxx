/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQBenchmarkSampler.cpp
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include "FairMQBenchmarkSampler.h"

#include <vector>
#include <chrono>

#include "../FairMQLogger.h"
#include "../options/FairMQProgOptions.h"

using namespace std;

FairMQBenchmarkSampler::FairMQBenchmarkSampler()
    : fSameMessage(true)
    , fMsgSize(10000)
    , fMsgCounter(0)
    , fMsgRate(1)
    , fNumIterations(0)
    , fMaxIterations(0)
    , fOutChannelName()
    , fResetMsgCounter()
{
}

FairMQBenchmarkSampler::~FairMQBenchmarkSampler()
{
}

void FairMQBenchmarkSampler::InitTask()
{
    fSameMessage = fConfig->GetValue<bool>("same-msg");
    fMsgSize = fConfig->GetValue<int>("msg-size");
    fMsgRate = fConfig->GetValue<int>("msg-rate");
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
    fOutChannelName = fConfig->GetValue<string>("out-channel");
}

void FairMQBenchmarkSampler::PreRun()
{
    fResetMsgCounter = std::thread(&FairMQBenchmarkSampler::ResetMsgCounter, this);
}

void FairMQBenchmarkSampler::Run()
{
    // store the channel reference to avoid traversing the map on every loop iteration
    FairMQChannel& dataOutChannel = fChannels.at(fOutChannelName).at(0);

    FairMQMessagePtr baseMsg(dataOutChannel.Transport()->CreateMessage(fMsgSize));

    LOG(info) << "Starting the benchmark with message size of " << fMsgSize << " and " << fMaxIterations << " iterations.";
    auto tStart = chrono::high_resolution_clock::now();

    while (CheckCurrentState(RUNNING))
    {
        if (fSameMessage)
        {
            FairMQMessagePtr msg(dataOutChannel.Transport()->CreateMessage());
            msg->Copy(*baseMsg);

            if (dataOutChannel.Send(msg) >= 0)
            {
                if (fMaxIterations > 0)
                {
                    if (fNumIterations >= fMaxIterations)
                    {
                        break;
                    }
                }
                ++fNumIterations;
            }
        }
        else
        {
            FairMQMessagePtr msg(dataOutChannel.Transport()->CreateMessage(fMsgSize));

            if (dataOutChannel.Send(msg) >= 0)
            {
                if (fMaxIterations > 0)
                {
                    if (fNumIterations >= fMaxIterations)
                    {
                        break;
                    }
                }
                ++fNumIterations;
            }
        }

        --fMsgCounter;

        while (fMsgCounter == 0)
        {
            this_thread::sleep_for(chrono::milliseconds(1));
        }
    }

    auto tEnd = chrono::high_resolution_clock::now();

    LOG(info) << "Done " << fNumIterations << " iterations in " << chrono::duration<double, milli>(tEnd - tStart).count() << "ms.";

}

void FairMQBenchmarkSampler::PostRun()
{
    fResetMsgCounter.join();
}

void FairMQBenchmarkSampler::ResetMsgCounter()
{
    while (CheckCurrentState(RUNNING))
    {
        fMsgCounter = fMsgRate / 100;
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    fMsgCounter = -1;
}

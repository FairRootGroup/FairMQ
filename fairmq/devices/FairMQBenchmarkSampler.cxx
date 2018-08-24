/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQBenchmarkSampler.h"

#include <fairmq/Tools.h>
#include "../FairMQLogger.h"
#include "../options/FairMQProgOptions.h"

#include <vector>
#include <chrono>

using namespace std;

FairMQBenchmarkSampler::FairMQBenchmarkSampler()
    : fSameMessage(true)
    , fMsgSize(10000)
    , fMsgRate(0)
    , fNumIterations(0)
    , fMaxIterations(0)
    , fOutChannelName()
{
}

FairMQBenchmarkSampler::~FairMQBenchmarkSampler()
{
}

void FairMQBenchmarkSampler::InitTask()
{
    fSameMessage = fConfig->GetValue<bool>("same-msg");
    fMsgSize = fConfig->GetValue<int>("msg-size");
    fMsgRate = fConfig->GetValue<float>("msg-rate");
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
    fOutChannelName = fConfig->GetValue<string>("out-channel");
}

void FairMQBenchmarkSampler::Run()
{
    // store the channel reference to avoid traversing the map on every loop iteration
    FairMQChannel& dataOutChannel = fChannels.at(fOutChannelName).at(0);

    FairMQMessagePtr baseMsg(dataOutChannel.NewMessage(fMsgSize));

    LOG(info) << "Starting the benchmark with message size of " << fMsgSize << " and " << fMaxIterations << " iterations.";
    auto tStart = chrono::high_resolution_clock::now();

    fair::mq::tools::RateLimiter rateLimiter(fMsgRate);

    while (CheckCurrentState(RUNNING))
    {
        if (fSameMessage)
        {
            FairMQMessagePtr msg(dataOutChannel.NewMessage());
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
            FairMQMessagePtr msg(dataOutChannel.NewMessage(fMsgSize));

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


        if (fMsgRate > 0)
        {
            rateLimiter.maybe_sleep();
        }
    }

    auto tEnd = chrono::high_resolution_clock::now();

    LOG(info) << "Done " << fNumIterations << " iterations in " << chrono::duration<double, milli>(tEnd - tStart).count() << "ms.";
}

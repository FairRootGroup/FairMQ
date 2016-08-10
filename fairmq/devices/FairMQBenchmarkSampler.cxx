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

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/timer/timer.hpp>

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
    // boost::thread resetMsgCounter(boost::bind(&FairMQBenchmarkSampler::ResetMsgCounter, this));

    int numSentMsgs = 0;

    FairMQMessagePtr baseMsg(fTransportFactory->CreateMessage(fMsgSize));

    // store the channel reference to avoid traversing the map on every loop iteration
    const FairMQChannel& dataOutChannel = fChannels.at(fOutChannelName).at(0);

    LOG(INFO) << "Starting the benchmark with message size of " << fMsgSize << " and number of messages " << fNumMsgs << ".";
    boost::timer::auto_cpu_timer timer;

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
        //   boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        // }
    }

    LOG(INFO) << "Sent " << numSentMsgs << " messages, leaving RUNNING state.";
    LOG(INFO) << "Sending time: ";

    // try
    // {
    //     resetMsgCounter.interrupt();
    //     resetMsgCounter.join();
    // }
    // catch(boost::thread_resource_error& e)
    // {
    //     LOG(ERROR) << e.what();
    //     exit(EXIT_FAILURE);
    // }
}

void FairMQBenchmarkSampler::ResetMsgCounter()
{
    while (true)
    {
        try
        {
            fMsgCounter = fMsgRate / 100;
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        }
        catch (boost::thread_interrupted&)
        {
            LOG(DEBUG) << "Event rate limiter thread interrupted";
            break;
        }
    }
}

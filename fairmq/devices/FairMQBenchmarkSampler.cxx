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

using namespace std;

FairMQBenchmarkSampler::FairMQBenchmarkSampler()
    : fMsgSize(10000)
    , fNumMsgs(0)
{
}

FairMQBenchmarkSampler::~FairMQBenchmarkSampler()
{
}

void FairMQBenchmarkSampler::Run()
{
    void* buffer = malloc(fMsgSize);
    int numSentMsgs = 0;

    unique_ptr<FairMQMessage> baseMsg(fTransportFactory->CreateMessage(buffer, fMsgSize));

    // store the channel reference to avoid traversing the map on every loop iteration
    const FairMQChannel& dataOutChannel = fChannels.at("data-out").at(0);

    LOG(INFO) << "Starting the benchmark with message size of " << fMsgSize << " and number of messages " << fNumMsgs << ".";
    boost::timer::auto_cpu_timer timer;

    while (CheckCurrentState(RUNNING))
    {
        unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());
        msg->Copy(baseMsg);

        if (dataOutChannel.Send(msg) >= 0)
        {
            if (fNumMsgs > 0)
            {
                numSentMsgs++;
                if (numSentMsgs >= fNumMsgs)
                {
                    break;
                }
            }
        }
    }

    LOG(INFO) << "Sent " << numSentMsgs << " messages, leaving RUNNING state.";
    LOG(INFO) << "Sending time: ";
}

void FairMQBenchmarkSampler::SetProperty(const int key, const string& value)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

string FairMQBenchmarkSampler::GetProperty(const int key, const string& default_ /*= ""*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

void FairMQBenchmarkSampler::SetProperty(const int key, const int value)
{
    switch (key)
    {
        case MsgSize:
            fMsgSize = value;
            break;
        case NumMsgs:
            fNumMsgs = value;
            break;
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

int FairMQBenchmarkSampler::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        case MsgSize:
            return fMsgSize;
        case NumMsgs:
            return fNumMsgs;
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

string FairMQBenchmarkSampler::GetPropertyDescription(const int key)
{
    switch (key)
    {
        case MsgSize:
            return "MsgSize: Size of the transfered message buffer.";
        case NumMsgs:
            return "NumMsgs: Number of messages to send.";
        default:
            return FairMQDevice::GetPropertyDescription(key);
    }
}

void FairMQBenchmarkSampler::ListProperties()
{
    LOG(INFO) << "Properties of FairMQBenchmarkSampler:";
    for (int p = FairMQConfigurable::Last; p < FairMQBenchmarkSampler::Last; ++p)
    {
        LOG(INFO) << " " << GetPropertyDescription(p);
    }
    LOG(INFO) << "---------------------------";
}

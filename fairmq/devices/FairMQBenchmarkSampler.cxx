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

#include "FairMQBenchmarkSampler.h"
#include "FairMQLogger.h"

using namespace std;

FairMQBenchmarkSampler::FairMQBenchmarkSampler()
    : fEventSize(10000)
    , fEventRate(1)
    , fEventCounter(0)
{
}

FairMQBenchmarkSampler::~FairMQBenchmarkSampler()
{
}

void FairMQBenchmarkSampler::Run()
{
    boost::thread resetEventCounter(boost::bind(&FairMQBenchmarkSampler::ResetEventCounter, this));

    void* buffer = operator new[](fEventSize);
    FairMQMessage* baseMsg = fTransportFactory->CreateMessage(buffer, fEventSize);

    // store the channel reference to avoid traversing the map on every loop iteration
    const FairMQChannel& dataChannel = fChannels.at("data-out").at(0);

    while (CheckCurrentState(RUNNING))
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();
        msg->Copy(baseMsg);

        dataChannel.Send(msg);

        --fEventCounter;

        while (fEventCounter == 0)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }

        delete msg;
    }

    delete baseMsg;

    try {
        resetEventCounter.interrupt();
        resetEventCounter.join();
    } catch(boost::thread_resource_error& e) {
        LOG(ERROR) << e.what();
    }
}

void FairMQBenchmarkSampler::ResetEventCounter()
{
    while (CheckCurrentState(RUNNING))
    {
        try
        {
            fEventCounter = fEventRate / 100;
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        }
        catch (boost::thread_interrupted&)
        {
            break;
        }
    }
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
        case EventSize:
            fEventSize = value;
            break;
        case EventRate:
            fEventRate = value;
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
        case EventSize:
            return fEventSize;
        case EventRate:
            return fEventRate;
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

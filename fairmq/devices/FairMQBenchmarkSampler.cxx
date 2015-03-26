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

void FairMQBenchmarkSampler::Init()
{
    FairMQDevice::Init();
}

void FairMQBenchmarkSampler::Run()
{
    LOG(INFO) << ">>>>>>> Run <<<<<<<";

    boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));
    boost::thread resetEventCounter(boost::bind(&FairMQBenchmarkSampler::ResetEventCounter, this));

    void* buffer = operator new[](fEventSize);
    FairMQMessage* base_msg = fTransportFactory->CreateMessage(buffer, fEventSize);

    while (fState == RUNNING)
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();
        msg->Copy(base_msg);

        fPayloadOutputs->at(0)->Send(msg);

        --fEventCounter;

        while (fEventCounter == 0)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }

        delete msg;
    }

    delete base_msg;

    try {
        rateLogger.interrupt();
        rateLogger.join();
        resetEventCounter.interrupt();
        resetEventCounter.join();
    } catch(boost::thread_resource_error& e) {
        LOG(ERROR) << e.what();
    }

    FairMQDevice::Shutdown();

    // notify parent thread about end of processing.
    boost::lock_guard<boost::mutex> lock(fRunningMutex);
    fRunningFinished = true;
    fRunningCondition.notify_one();
}

void FairMQBenchmarkSampler::ResetEventCounter()
{
    while (true)
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

void FairMQBenchmarkSampler::Log(int intervalInMs)
{
    timestamp_t t0;
    timestamp_t t1;
    unsigned long bytes = fPayloadOutputs->at(0)->GetBytesTx();
    unsigned long messages = fPayloadOutputs->at(0)->GetMessagesTx();
    unsigned long bytesNew = 0;
    unsigned long messagesNew = 0;
    double megabytesPerSecond = 0;
    double messagesPerSecond = 0;

    t0 = get_timestamp();

    while (true)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(intervalInMs));

        t1 = get_timestamp();

        bytesNew = fPayloadOutputs->at(0)->GetBytesTx();
        messagesNew = fPayloadOutputs->at(0)->GetMessagesTx();

        timestamp_t timeSinceLastLog_ms = (t1 - t0) / 1000.0L;

        megabytesPerSecond = ((double)(bytesNew - bytes) / (1024. * 1024.)) / (double)timeSinceLastLog_ms * 1000.;
        messagesPerSecond = (double)(messagesNew - messages) / (double)timeSinceLastLog_ms * 1000.;

        LOG(DEBUG) << "send " << messagesPerSecond << " msg/s, " << megabytesPerSecond << " MB/s";

        bytes = bytesNew;
        messages = messagesNew;
        t0 = t1;
    }
}

void FairMQBenchmarkSampler::SetProperty(const int key, const string& value, const int slot /*= 0*/)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value, slot);
            break;
    }
}

string FairMQBenchmarkSampler::GetProperty(const int key, const string& default_ /*= ""*/, const int slot /*= 0*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_, slot);
    }
}

void FairMQBenchmarkSampler::SetProperty(const int key, const int value, const int slot /*= 0*/)
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
            FairMQDevice::SetProperty(key, value, slot);
            break;
    }
}

int FairMQBenchmarkSampler::GetProperty(const int key, const int default_ /*= 0*/, const int slot /*= 0*/)
{
    switch (key)
    {
        case EventSize:
            return fEventSize;
        case EventRate:
            return fEventRate;
        default:
            return FairMQDevice::GetProperty(key, default_, slot);
    }
}

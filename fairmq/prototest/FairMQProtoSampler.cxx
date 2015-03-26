/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQProtoSampler.cpp
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <string>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQProtoSampler.h"
#include "FairMQLogger.h"

#include "payload.pb.h"

using namespace std;

FairMQProtoSampler::FairMQProtoSampler()
    : fEventSize(10000)
    , fEventRate(1)
    , fEventCounter(0)
{
}

FairMQProtoSampler::~FairMQProtoSampler()
{
}

void FairMQProtoSampler::Init()
{
    FairMQDevice::Init();
}

void FairMQProtoSampler::Run()
{
    LOG(INFO) << ">>>>>>> Run <<<<<<<";
    // boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

    boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));
    boost::thread resetEventCounter(boost::bind(&FairMQProtoSampler::ResetEventCounter, this));

    srand(time(NULL));

    while (fState == RUNNING)
    {

        sampler::Payload p;

        for (int i = 0; i < fEventSize; ++i)
        {
            sampler::Content* content = p.add_data();

            content->set_x(rand() % 100 + 1);
            content->set_y(rand() % 100 + 1);
            content->set_z(rand() % 100 + 1);
            content->set_a((rand() % 100 + 1) / (rand() % 100 + 1));
            content->set_b((rand() % 100 + 1) / (rand() % 100 + 1));
            // LOG(INFO) << content->x() << " " << content->y() << " " << content->z() << " " << content->a() << " " << content->b();
        }

        string str;
        p.SerializeToString(&str);
        size_t size = str.length();

        FairMQMessage* msg = fTransportFactory->CreateMessage(size);
        memcpy(msg->GetData(), str.c_str(), size);

        fPayloadOutputs->at(0)->Send(msg);

        --fEventCounter;

        while (fEventCounter == 0)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }

        delete msg;
    }

    rateLogger.interrupt();
    resetEventCounter.interrupt();

    rateLogger.join();
    resetEventCounter.join();
}

void FairMQProtoSampler::ResetEventCounter()
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

void FairMQProtoSampler::Log(int intervalInMs)
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

void FairMQProtoSampler::SetProperty(const int key, const string& value, const int slot /*= 0*/)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value, slot);
            break;
    }
}

string FairMQProtoSampler::GetProperty(const int key, const string& default_ /*= ""*/, const int slot /*= 0*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_, slot);
    }
}

void FairMQProtoSampler::SetProperty(const int key, const int value, const int slot /*= 0*/)
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

int FairMQProtoSampler::GetProperty(const int key, const int default_ /*= 0*/, const int slot /*= 0*/)
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

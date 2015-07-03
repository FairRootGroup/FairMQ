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

void FairMQProtoSampler::Run()
{
    boost::thread resetEventCounter(boost::bind(&FairMQProtoSampler::ResetEventCounter, this));

    srand(time(NULL));

    const FairMQChannel& dataOutChannel = fChannels.at("data-out").at(0);

    while (CheckCurrentState(RUNNING))
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

        dataOutChannel.Send(msg);

        --fEventCounter;

        while (fEventCounter == 0)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }

        delete msg;
    }

    resetEventCounter.interrupt();
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

void FairMQProtoSampler::SetProperty(const int key, const string& value)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

string FairMQProtoSampler::GetProperty(const int key, const string& default_ /*= ""*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

void FairMQProtoSampler::SetProperty(const int key, const int value)
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

int FairMQProtoSampler::GetProperty(const int key, const int default_ /*= 0*/)
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

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQBinSampler.cpp
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <vector>
#include <stdlib.h> /* srand, rand */
#include <time.h>   /* time */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQBinSampler.h"
#include "FairMQLogger.h"

using namespace std;

FairMQBinSampler::FairMQBinSampler()
    : fEventSize(10000)
    , fEventRate(1)
    , fEventCounter(0)
{
}

FairMQBinSampler::~FairMQBinSampler()
{
}

void FairMQBinSampler::Run()
{
    boost::thread resetEventCounter(boost::bind(&FairMQBinSampler::ResetEventCounter, this));

    srand(time(NULL));

    LOG(DEBUG) << "Message size: " << fEventSize * sizeof(Content) << " bytes.";

    const FairMQChannel& dataOutChannel = fChannels.at("data-out").at(0);

    while (CheckCurrentState(RUNNING))
    {
        Content* payload = new Content[fEventSize];

        for (int i = 0; i < fEventSize; ++i)
        {
            (&payload[i])->x = rand() % 100 + 1;
            (&payload[i])->y = rand() % 100 + 1;
            (&payload[i])->z = rand() % 100 + 1;
            (&payload[i])->a = (rand() % 100 + 1) / (rand() % 100 + 1);
            (&payload[i])->b = (rand() % 100 + 1) / (rand() % 100 + 1);
            // LOG(INFO) << (&payload[i])->x << " " << (&payload[i])->y << " " << (&payload[i])->z << " " << (&payload[i])->a << " " << (&payload[i])->b;
        }

        FairMQMessage* msg = fTransportFactory->CreateMessage(fEventSize * sizeof(Content));
        memcpy(msg->GetData(), payload, fEventSize * sizeof(Content));

        dataOutChannel.Send(msg);

        --fEventCounter;

        while (fEventCounter == 0)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }

        delete[] payload;
        delete msg;
    }

    resetEventCounter.interrupt();
    resetEventCounter.join();
}

void FairMQBinSampler::ResetEventCounter()
{
    while (GetCurrentState() == RUNNING)
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

void FairMQBinSampler::SetProperty(const int key, const string& value)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

string FairMQBinSampler::GetProperty(const int key, const string& default_ /*= ""*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

void FairMQBinSampler::SetProperty(const int key, const int value)
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

int FairMQBinSampler::GetProperty(const int key, const int default_ /*= 0*/)
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

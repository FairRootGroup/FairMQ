/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample6Sampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <memory> // unique_ptr

#include <boost/thread.hpp>

#include "FairMQExample6Sampler.h"
#include "FairMQPoller.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample6Sampler::FairMQExample6Sampler()
    : fText()
{
}

void FairMQExample6Sampler::CustomCleanup(void *data, void *object)
{
    delete (string*)object;
}

void FairMQExample6Sampler::Run()
{
    std::unique_ptr<FairMQPoller> poller(fTransportFactory->CreatePoller(fChannels, { "data", "broadcast" }));

    while (CheckCurrentState(RUNNING))
    {
        poller->Poll(-1);

        if (poller->CheckInput("broadcast", 0))
        {
            unique_ptr<FairMQMessage> msg(NewMessage());

            if (Receive(msg, "broadcast") > 0)
            {
                LOG(INFO) << "Received broadcast: \""
                          << string(static_cast<char*>(msg->GetData()), msg->GetSize())
                          << "\"";
            }
        }

        if (poller->CheckOutput("data", 0))
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

            string* text = new string(fText);

            unique_ptr<FairMQMessage> msg(NewMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text));

            LOG(INFO) << "Sending \"" << fText << "\"";

            Send(msg, "data");
        }
    }
}

FairMQExample6Sampler::~FairMQExample6Sampler()
{
}

void FairMQExample6Sampler::SetProperty(const int key, const string& value)
{
    switch (key)
    {
        case Text:
            fText = value;
            break;
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

string FairMQExample6Sampler::GetProperty(const int key, const string& default_ /*= ""*/)
{
    switch (key)
    {
        case Text:
            return fText;
            break;
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

void FairMQExample6Sampler::SetProperty(const int key, const int value)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

int FairMQExample6Sampler::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

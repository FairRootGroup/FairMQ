/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample2Sampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExample2Sampler.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample2Sampler::FairMQExample2Sampler()
    : fText()
{
}

void FairMQExample2Sampler::CustomCleanup(void *data, void *object)
{
    delete (string*)object;
}

void FairMQExample2Sampler::Run()
{
    // Check if we are still in the RUNNING state
    while (CheckCurrentState(RUNNING))
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        string* text = new string(fText);

        unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text));

        LOG(INFO) << "Sending \"" << fText << "\"";

        fChannels.at("data-out").at(0).Send(msg);
    }
}

FairMQExample2Sampler::~FairMQExample2Sampler()
{
}

void FairMQExample2Sampler::SetProperty(const int key, const string& value)
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

string FairMQExample2Sampler::GetProperty(const int key, const string& default_ /*= ""*/)
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

void FairMQExample2Sampler::SetProperty(const int key, const int value)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

int FairMQExample2Sampler::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

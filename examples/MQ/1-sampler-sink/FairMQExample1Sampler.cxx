/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample1Sampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <memory> // unique_ptr

#include <boost/thread.hpp>

#include "FairMQExample1Sampler.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample1Sampler::FairMQExample1Sampler()
    : fText()
{
}

void FairMQExample1Sampler::CustomCleanup(void* /*data*/, void *object)
{
    delete static_cast<string*>(object);
}

void FairMQExample1Sampler::Run()
{
    while (CheckCurrentState(RUNNING))
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        string* text = new string(fText);

        unique_ptr<FairMQMessage> msg(NewMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text));

        LOG(INFO) << "Sending \"" << fText << "\"";

        Send(msg, "data");
    }
}

FairMQExample1Sampler::~FairMQExample1Sampler()
{
}

void FairMQExample1Sampler::SetProperty(const int key, const string& value)
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

string FairMQExample1Sampler::GetProperty(const int key, const string& default_ /*= ""*/)
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

void FairMQExample1Sampler::SetProperty(const int key, const int value)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

int FairMQExample1Sampler::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

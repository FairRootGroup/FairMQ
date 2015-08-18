/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample5Client.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExample5Client.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample5Client::FairMQExample5Client()
    : fText()
{
}

FairMQExample5Client::~FairMQExample5Client()
{
}

void FairMQExample5Client::CustomCleanup(void *data, void *hint)
{
    delete (string*)hint;
}

void FairMQExample5Client::Run()
{
    while (CheckCurrentState(RUNNING))
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        string* text = new string(fText);

        FairMQMessage* request = fTransportFactory->CreateMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text);
        FairMQMessage* reply = fTransportFactory->CreateMessage();

        LOG(INFO) << "Sending \"" << fText << "\" to server.";

        if (fChannels.at("data").at(0).Send(request) > 0)
        {
            fChannels.at("data").at(0).Receive(reply);
        }

        LOG(INFO) << "Received reply from server: \"" << string(static_cast<char*>(reply->GetData()), reply->GetSize()) << "\"";

        delete reply;
    }
}


void FairMQExample5Client::SetProperty(const int key, const string& value)
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

string FairMQExample5Client::GetProperty(const int key, const string& default_ /*= ""*/)
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

void FairMQExample5Client::SetProperty(const int key, const int value)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

int FairMQExample5Client::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

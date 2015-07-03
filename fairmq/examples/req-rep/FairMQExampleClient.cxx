/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExampleClient.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExampleClient.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExampleClient::FairMQExampleClient()
    : fText()
{
}

FairMQExampleClient::~FairMQExampleClient()
{
}

void FairMQExampleClient::CustomCleanup(void *data, void *hint)
{
    delete (string*)hint;
}

void FairMQExampleClient::Run()
{
    while (GetCurrentState() == RUNNING)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        string* text = new string(fText);

        FairMQMessage* request = fTransportFactory->CreateMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text);
        FairMQMessage* reply = fTransportFactory->CreateMessage();

        LOG(INFO) << "Sending \"" << fText << "\" to server.";

        fChannels.at("data").at(0).Send(request);
        fChannels.at("data").at(0).Receive(reply);

        LOG(INFO) << "Received reply from server: \"" << string(static_cast<char*>(reply->GetData()), reply->GetSize()) << "\"";

        delete reply;
    }
}


void FairMQExampleClient::SetProperty(const int key, const string& value)
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

string FairMQExampleClient::GetProperty(const int key, const string& default_ /*= ""*/)
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

void FairMQExampleClient::SetProperty(const int key, const int value)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

int FairMQExampleClient::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

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
    // boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

    while (fState == RUNNING)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        string* text = new string(fText);

        FairMQMessage* request = fTransportFactory->CreateMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text);
        FairMQMessage* reply = fTransportFactory->CreateMessage();

        LOG(INFO) << "Sending \"" << fText << "\" to server.";

        fPayloadOutputs->at(0)->Send(request);
        fPayloadOutputs->at(0)->Receive(reply);

        LOG(INFO) << "Received reply from server: \"" << string(static_cast<char*>(reply->GetData()), reply->GetSize()) << "\"";

        delete reply;
    }

    // rateLogger.interrupt();
    // rateLogger.join();

    FairMQDevice::Shutdown();

    boost::lock_guard<boost::mutex> lock(fRunningMutex);
    fRunningFinished = true;
    fRunningCondition.notify_one();
}


void FairMQExampleClient::SetProperty(const int key, const string& value, const int slot /*= 0*/)
{
    switch (key)
    {
        case Text:
            fText = value;
            break;
        default:
            FairMQDevice::SetProperty(key, value, slot);
            break;
    }
}

string FairMQExampleClient::GetProperty(const int key, const string& default_ /*= ""*/, const int slot /*= 0*/)
{
    switch (key)
    {
        case Text:
            return fText;
            break;
        default:
            return FairMQDevice::GetProperty(key, default_, slot);
    }
}

void FairMQExampleClient::SetProperty(const int key, const int value, const int slot /*= 0*/)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value, slot);
            break;
    }
}

int FairMQExampleClient::GetProperty(const int key, const int default_ /*= 0*/, const int slot /*= 0*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_, slot);
    }
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQMerger.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQLogger.h"
#include "FairMQMerger.h"
#include "FairMQPoller.h"

using namespace std;

FairMQMerger::FairMQMerger()
    : fMultipart(1)
{
}

FairMQMerger::~FairMQMerger()
{
}

void FairMQMerger::Run()
{
    int numInputs = fChannels.at("data-in").size();

    std::unique_ptr<FairMQPoller> poller(fTransportFactory->CreatePoller(fChannels.at("data-in")));

    if (fMultipart)
    {
        while (CheckCurrentState(RUNNING))
        {
            poller->Poll(100);

            // Loop over the data input channels.
            for (int i = 0; i < numInputs; ++i)
            {
                // Check if the channel has data ready to be received.
                if (poller->CheckInput(i))
                {
                    FairMQParts payload;

                    if (Receive(payload, "data-in", i) >= 0)
                    {
                        if (Send(payload, "data-out") < 0)
                        {
                            LOG(DEBUG) << "Transfer interrupted";
                            break;
                        }
                    }
                    else
                    {
                        LOG(DEBUG) << "Transfer interrupted";
                        break;
                    }
                }
            }
        }
    }
    else
    {
        while (CheckCurrentState(RUNNING))
        {
            poller->Poll(100);

            // Loop over the data input channels.
            for (int i = 0; i < numInputs; ++i)
            {
                // Check if the channel has data ready to be received.
                if (poller->CheckInput(i))
                {
                    unique_ptr<FairMQMessage> payload(fTransportFactory->CreateMessage());

                    if (Receive(payload, "data-in", i) >= 0)
                    {
                        if (Send(payload, "data-out") < 0)
                        {
                            LOG(DEBUG) << "Transfer interrupted";
                            break;
                        }
                    }
                    else
                    {
                        LOG(DEBUG) << "Transfer interrupted";
                        break;
                    }
                }
            }
        }
    }
}

void FairMQMerger::SetProperty(const int key, const string& value)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

string FairMQMerger::GetProperty(const int key, const string& default_ /*= ""*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

void FairMQMerger::SetProperty(const int key, const int value)
{
    switch (key)
    {
        case Multipart:
            fMultipart = value;
            break;
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

int FairMQMerger::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        case Multipart:
            return fMultipart;
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

string FairMQMerger::GetPropertyDescription(const int key)
{
    switch (key)
    {
        case Multipart:
            return "Multipart: Handle payloads as multipart messages.";
        default:
            return FairMQDevice::GetPropertyDescription(key);
    }
}

void FairMQMerger::ListProperties()
{
    LOG(INFO) << "Properties of FairMQMerger:";
    for (int p = FairMQConfigurable::Last; p < FairMQMerger::Last; ++p)
    {
        LOG(INFO) << " " << GetPropertyDescription(p);
    }
    LOG(INFO) << "---------------------------";
}

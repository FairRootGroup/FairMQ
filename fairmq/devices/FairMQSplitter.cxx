/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSplitter.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQLogger.h"
#include "FairMQSplitter.h"

using namespace std;

FairMQSplitter::FairMQSplitter()
    : fMultipart(1)
{
}

FairMQSplitter::~FairMQSplitter()
{
}

void FairMQSplitter::Run()
{
    int numOutputs = fChannels.at("data-out").size();

    int direction = 0;

    if (fMultipart)
    {
        while (CheckCurrentState(RUNNING))
        {
            FairMQParts payload;

            if (Receive(payload, "data-in") >= 0)
            {
                if (Send(payload, "data-out", direction) < 0)
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

            ++direction;
            if (direction >= numOutputs)
            {
                direction = 0;
            }
        }
    }
    else
    {
        while (CheckCurrentState(RUNNING))
        {
            unique_ptr<FairMQMessage> payload(fTransportFactory->CreateMessage());

            if (Receive(payload, "data-in") >= 0)
            {
                if (Send(payload, "data-out", direction) < 0)
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

            ++direction;
            if (direction >= numOutputs)
            {
                direction = 0;
            }
        }
    }
}

void FairMQSplitter::SetProperty(const int key, const string& value)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

string FairMQSplitter::GetProperty(const int key, const string& default_ /*= ""*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

void FairMQSplitter::SetProperty(const int key, const int value)
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

int FairMQSplitter::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        case Multipart:
            return fMultipart;
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

string FairMQSplitter::GetPropertyDescription(const int key)
{
    switch (key)
    {
        case Multipart:
            return "Multipart: Handle payloads as multipart messages.";
        default:
            return FairMQDevice::GetPropertyDescription(key);
    }
}

void FairMQSplitter::ListProperties()
{
    LOG(INFO) << "Properties of FairMQSplitter:";
    for (int p = FairMQConfigurable::Last; p < FairMQSplitter::Last; ++p)
    {
        LOG(INFO) << " " << GetPropertyDescription(p);
    }
    LOG(INFO) << "---------------------------";
}


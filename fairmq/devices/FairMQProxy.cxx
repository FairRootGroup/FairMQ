/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQProxy.cxx
 *
 * @since 2013-10-02
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQLogger.h"
#include "FairMQProxy.h"

using namespace std;

FairMQProxy::FairMQProxy()
    : fMultipart(1)
{
}

FairMQProxy::~FairMQProxy()
{
}

void FairMQProxy::Run()
{
    if (fMultipart)
    {
        while (CheckCurrentState(RUNNING))
        {
            FairMQParts payload;
            if (Receive(payload, "data-in") >= 0)
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
    else
    {
        while (CheckCurrentState(RUNNING))
        {
            unique_ptr<FairMQMessage> payload(fTransportFactory->CreateMessage());
            if (Receive(payload, "data-in") >= 0)
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

void FairMQProxy::SetProperty(const int key, const string& value)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

string FairMQProxy::GetProperty(const int key, const string& default_ /*= ""*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

void FairMQProxy::SetProperty(const int key, const int value)
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

int FairMQProxy::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        case Multipart:
            return fMultipart;
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

string FairMQProxy::GetPropertyDescription(const int key)
{
    switch (key)
    {
        case Multipart:
            return "Multipart: Handle payloads as multipart messages.";
        default:
            return FairMQDevice::GetPropertyDescription(key);
    }
}

void FairMQProxy::ListProperties()
{
    LOG(INFO) << "Properties of FairMQProxy:";
    for (int p = FairMQConfigurable::Last; p < FairMQProxy::Last; ++p)
    {
        LOG(INFO) << " " << GetPropertyDescription(p);
    }
    LOG(INFO) << "---------------------------";
}


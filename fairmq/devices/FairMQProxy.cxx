/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQProxy.cxx
 *
 * @since 2013-10-02
 * @author A. Rybalchenko
 */

#include "FairMQProxy.h"

#include "../FairMQLogger.h"

using namespace std;

FairMQProxy::FairMQProxy()
    : fMultipart(true)
    , fInChannelName()
    , fOutChannelName()
{
}

FairMQProxy::~FairMQProxy()
{
}

void FairMQProxy::InitTask()
{
    fMultipart = fConfig->GetProperty<bool>("multipart");
    fInChannelName = fConfig->GetProperty<string>("in-channel");
    fOutChannelName = fConfig->GetProperty<string>("out-channel");
}

void FairMQProxy::Run()
{
    if (fMultipart)
    {
        while (!NewStatePending())
        {
            FairMQParts payload;
            if (Receive(payload, fInChannelName) >= 0)
            {
                if (Send(payload, fOutChannelName) < 0)
                {
                    LOG(debug) << "Transfer interrupted";
                    break;
                }
            }
            else
            {
                LOG(debug) << "Transfer interrupted";
                break;
            }
        }
    }
    else
    {
        while (!NewStatePending())
        {
            unique_ptr<FairMQMessage> payload(fTransportFactory->CreateMessage());
            if (Receive(payload, fInChannelName) >= 0)
            {
                if (Send(payload, fOutChannelName) < 0)
                {
                    LOG(debug) << "Transfer interrupted";
                    break;
                }
            }
            else
            {
                LOG(debug) << "Transfer interrupted";
                break;
            }
        }
    }
}

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
#include "../options/FairMQProgOptions.h"

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
    fMultipart = fConfig->GetValue<bool>("multipart");
    fInChannelName = fConfig->GetValue<string>("in-channel");
    fOutChannelName = fConfig->GetValue<string>("out-channel");
}

void FairMQProxy::Run()
{
    if (fMultipart)
    {
        while (CheckCurrentState(RUNNING))
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
        while (CheckCurrentState(RUNNING))
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

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

FairMQProxy::FairMQProxy()
{
}

FairMQProxy::~FairMQProxy()
{
}

void FairMQProxy::Run()
{
    FairMQMessage* msg = fTransportFactory->CreateMessage();

    // store the channel references to avoid traversing the map on every loop iteration
    const FairMQChannel& dataInChannel = fChannels.at("data-in").at(0);
    const FairMQChannel& dataOutChannel = fChannels.at("data-out").at(0);

    while (CheckCurrentState(RUNNING))
    {
        if (dataInChannel.Receive(msg) > 0)
        {
            dataOutChannel.Send(msg);
        }
    }

    delete msg;
}

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

    while (GetCurrentState() == RUNNING)
    {
        if (fChannels["data-in"].at(0).Receive(msg) > 0)
        {
            fChannels["data-out"].at(0).Send(msg);
        }
    }

    delete msg;
}

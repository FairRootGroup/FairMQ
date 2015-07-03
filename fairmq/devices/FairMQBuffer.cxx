/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQBuffer.cxx
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQBuffer.h"
#include "FairMQLogger.h"

FairMQBuffer::FairMQBuffer()
{
}

void FairMQBuffer::Run()
{
    // store the channel references to avoid traversing the map on every loop iteration
    const FairMQChannel& dataInChannel = fChannels.at("data-in").at(0);
    const FairMQChannel& dataOutChannel = fChannels.at("data-out").at(0);

    while (CheckCurrentState(RUNNING))
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();

        if (dataInChannel.Receive(msg) > 0)
        {
            dataOutChannel.Send(msg);
        }

        delete msg;
    }
}

FairMQBuffer::~FairMQBuffer()
{
}

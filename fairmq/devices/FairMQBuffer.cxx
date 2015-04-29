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
    while (GetCurrentState() == RUNNING)
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();

        if (fChannels["data-in"].at(0).Receive(msg) > 0)
        {
            fChannels["data-out"].at(0).Send(msg);
        }

        delete msg;
    }
}

FairMQBuffer::~FairMQBuffer()
{
}

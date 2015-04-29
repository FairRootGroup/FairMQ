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

FairMQSplitter::FairMQSplitter()
{
}

FairMQSplitter::~FairMQSplitter()
{
}

void FairMQSplitter::Run()
{
    int direction = 0;

    while (GetCurrentState() == RUNNING)
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();

        if (fChannels["data-in"].at(0).Receive(msg) > 0)
        {
            fChannels["data-out"].at(direction).Send(msg);
            direction++;
            if (direction >= fChannels["data-out"].size())
            {
                direction = 0;
            }
        }

        delete msg;
    }
}

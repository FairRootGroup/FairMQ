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

FairMQMerger::FairMQMerger()
{
}

FairMQMerger::~FairMQMerger()
{
}

void FairMQMerger::Run()
{
    FairMQPoller* poller = fTransportFactory->CreatePoller(fChannels["data-in"]);

    while (GetCurrentState() == RUNNING)
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();

        poller->Poll(100);

        for (int i = 0; i < fChannels["data-in"].size(); ++i)
        {
            if (poller->CheckInput(i))
            {
                if (fChannels["data-in"].at(i).Receive(msg))
                {
                    fChannels["data-out"].at(0).Send(msg);
                }
            }
        }

        delete msg;
    }

    delete poller;
}

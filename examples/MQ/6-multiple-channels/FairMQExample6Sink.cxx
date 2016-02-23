/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample6Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "FairMQExample6Sink.h"
#include "FairMQPoller.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample6Sink::FairMQExample6Sink()
{
}

void FairMQExample6Sink::Run()
{
    std::unique_ptr<FairMQPoller> poller(fTransportFactory->CreatePoller(fChannels, { "data", "broadcast" }));

    while (CheckCurrentState(RUNNING))
    {
        poller->Poll(-1);

        if (poller->CheckInput("broadcast", 0))
        {
            unique_ptr<FairMQMessage> msg(NewMessage());

            if (Receive(msg, "broadcast") > 0)
            {
                LOG(INFO) << "Received broadcast: \"" << string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";
            }
        }

        if (poller->CheckInput("data", 0))
        {
            unique_ptr<FairMQMessage> msg(NewMessage());

            if (Receive(msg, "data") > 0)
            {
                LOG(INFO) << "Received message: \"" << string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";
            }
        }
    }
}

FairMQExample6Sink::~FairMQExample6Sink()
{
}

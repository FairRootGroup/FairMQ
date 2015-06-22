/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample1Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExample1Sink.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample1Sink::FairMQExample1Sink()
{
}

void FairMQExample1Sink::Run()
{
    while (GetCurrentState() == RUNNING)
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();

        fChannels["data-in"].at(0).Receive(msg);

        LOG(INFO) << "Received message: \""
                  << string(static_cast<char*>(msg->GetData()), msg->GetSize())
                  << "\"";

        delete msg;
    }
}

FairMQExample1Sink::~FairMQExample1Sink()
{
}

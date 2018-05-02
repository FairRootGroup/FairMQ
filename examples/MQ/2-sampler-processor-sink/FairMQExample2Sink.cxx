/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample2Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExample2Sink.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample2Sink::FairMQExample2Sink()
{
}

void FairMQExample2Sink::Run()
{
    while (CheckCurrentState(RUNNING))
    {
        unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());

        if (fChannels.at("data-in").at(0).Receive(msg) >= 0)
        {
            LOG(INFO) << "Received message: \""
                      << string(static_cast<char*>(msg->GetData()), msg->GetSize())
                      << "\"";
        }
    }
}

FairMQExample2Sink::~FairMQExample2Sink()
{
}

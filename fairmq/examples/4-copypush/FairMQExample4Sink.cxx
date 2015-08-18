/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample4Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <memory>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExample4Sink.h"
#include "FairMQLogger.h"

FairMQExample4Sink::FairMQExample4Sink()
{
}

void FairMQExample4Sink::Run()
{
    while (CheckCurrentState(RUNNING))
    {
        std::unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());

        fChannels.at("data-in").at(0).Receive(msg);

        LOG(INFO) << "Received message: \"" << *(static_cast<int*>(msg->GetData())) << "\"";
    }
}

FairMQExample4Sink::~FairMQExample4Sink()
{
}

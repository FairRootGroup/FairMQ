/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTestPub.cpp
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include <memory> // unique_ptr

#include "FairMQTestPub.h"
#include "FairMQLogger.h"

FairMQTestPub::FairMQTestPub()
{
}

void FairMQTestPub::Run()
{
    std::unique_ptr<FairMQMessage> ready1Msg(fTransportFactory->CreateMessage());
    fChannels.at("control").at(0).Receive(ready1Msg);
    std::unique_ptr<FairMQMessage> ready2Msg(fTransportFactory->CreateMessage());
    fChannels.at("control").at(0).Receive(ready2Msg);

    std::unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());
    fChannels.at("data").at(0).Send(msg);

    std::unique_ptr<FairMQMessage> ack1Msg(fTransportFactory->CreateMessage());
    std::unique_ptr<FairMQMessage> ack2Msg(fTransportFactory->CreateMessage());
    if (fChannels.at("control").at(0).Receive(ack1Msg) >= 0)
    {
        if (fChannels.at("control").at(0).Receive(ack2Msg) >= 0)
        {
            LOG(INFO) << "PUB-SUB test successfull";
        }
    }
}

FairMQTestPub::~FairMQTestPub()
{
}

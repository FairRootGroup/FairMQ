/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTestRep.cpp
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include <memory> // unique_ptr

#include "FairMQTestRep.h"
#include "FairMQLogger.h"

FairMQTestRep::FairMQTestRep()
{
}

void FairMQTestRep::Run()
{
    std::unique_ptr<FairMQMessage> request(fTransportFactory->CreateMessage());
    if (fChannels.at("data").at(0).Receive(request) >= 0)
    {
        std::unique_ptr<FairMQMessage> reply(fTransportFactory->CreateMessage());
        fChannels.at("data").at(0).Send(reply);
    }
}

FairMQTestRep::~FairMQTestRep()
{
}

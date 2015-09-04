/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTestReq.cxx
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include <memory> // unique_ptr

#include "FairMQTestReq.h"
#include "FairMQLogger.h"

FairMQTestReq::FairMQTestReq()
{
}

void FairMQTestReq::Run()
{
    std::unique_ptr<FairMQMessage> request(fTransportFactory->CreateMessage());
    fChannels.at("data").at(0).Send(request);

    std::unique_ptr<FairMQMessage> reply(fTransportFactory->CreateMessage());
    if (fChannels.at("data").at(0).Receive(reply) >= 0)
    {
        LOG(INFO) << "REQ-REP test successfull";
    }
}

FairMQTestReq::~FairMQTestReq()
{
}

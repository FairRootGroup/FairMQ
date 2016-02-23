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
    std::unique_ptr<FairMQMessage> request(NewMessage());
    Send(request, "data");

    std::unique_ptr<FairMQMessage> reply(NewMessage());
    if (Receive(reply, "data") >= 0)
    {
        LOG(INFO) << "REQ-REP test successfull";
    }
}

FairMQTestReq::~FairMQTestReq()
{
}

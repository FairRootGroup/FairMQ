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

#include "FairMQTestRep.h"
#include "FairMQLogger.h"

FairMQTestRep::FairMQTestRep()
{
}

void FairMQTestRep::Run()
{
    FairMQMessagePtr request1(NewMessage());
    if (Receive(request1, "data") >= 0)
    {
        LOG(INFO) << "Received request 1";
        FairMQMessagePtr reply(NewMessage());
        Send(reply, "data");
    }
    FairMQMessagePtr request2(NewMessage());
    if (Receive(request2, "data") >= 0)
    {
        LOG(INFO) << "Received request 2";
        FairMQMessagePtr reply(NewMessage());
        Send(reply, "data");
    }

    LOG(INFO) << "REQ-REP test successfull";
}

FairMQTestRep::~FairMQTestRep()
{
}

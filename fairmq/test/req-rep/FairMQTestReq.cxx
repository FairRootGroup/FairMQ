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

#include "FairMQTestReq.h"
#include "FairMQLogger.h"

FairMQTestReq::FairMQTestReq()
{
}

void FairMQTestReq::Run()
{
    FairMQMessagePtr request(NewMessage());
    Send(request, "data");

    FairMQMessagePtr reply(NewMessage());
    if (Receive(reply, "data") >= 0)
    {
        LOG(INFO) << "received reply";
    }
}

FairMQTestReq::~FairMQTestReq()
{
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTestSub.cxx
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include <memory> // unique_ptr

#include "FairMQTestSub.h"
#include "FairMQLogger.h"

FairMQTestSub::FairMQTestSub()
{
}

void FairMQTestSub::Run()
{
    std::unique_ptr<FairMQMessage> readyMsg(NewMessage());
    Send(readyMsg, "control");

    std::unique_ptr<FairMQMessage> msg(NewMessage());
    if (Receive(msg, "data") >= 0)
    {
        std::unique_ptr<FairMQMessage> ackMsg(NewMessage());
        Send(ackMsg, "control");
    }
    else
    {
        LOG(ERROR) << "Test failed: size of the received message doesn't match. Expected: 0, Received: " << msg->GetSize();
    }
}

FairMQTestSub::~FairMQTestSub()
{
}

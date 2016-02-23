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
    std::unique_ptr<FairMQMessage> ready1Msg(NewMessage());
    int r1 = Receive(ready1Msg, "control");
    std::unique_ptr<FairMQMessage> ready2Msg(NewMessage());
    int r2 = Receive(ready2Msg, "control");

    if (r1 >= 0 && r2 >= 0)
    {
        std::unique_ptr<FairMQMessage> msg(NewMessage());
        Send(msg, "data");

        std::unique_ptr<FairMQMessage> ack1Msg(NewMessage());
        std::unique_ptr<FairMQMessage> ack2Msg(NewMessage());
        if (Receive(ack1Msg, "control") >= 0)
        {
            if (Receive(ack2Msg, "control") >= 0)
            {
                LOG(INFO) << "PUB-SUB test successfull";
            }
        }
    }
}

FairMQTestPub::~FairMQTestPub()
{
}

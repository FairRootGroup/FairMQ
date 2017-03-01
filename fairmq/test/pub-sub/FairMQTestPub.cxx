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

#include "FairMQTestPub.h"
#include "FairMQLogger.h"

FairMQTestPub::FairMQTestPub()
{
}

void FairMQTestPub::Run()
{
    FairMQMessagePtr ready1(NewMessage());
    FairMQMessagePtr ready2(NewMessage());
    int r1 = Receive(ready1, "control");
    int r2 = Receive(ready2, "control");
    if (r1 >= 0 && r2 >= 0)
    {
        LOG(INFO) << "Received both ready signals, proceeding to publish data";

        FairMQMessagePtr msg(NewMessage());
        int d1 = Send(msg, "data");
        if (d1 < 0)
        {
            LOG(ERROR) << "Failed sending data: d1 = " << d1;
        }

        FairMQMessagePtr ack1(NewMessage());
        FairMQMessagePtr ack2(NewMessage());
        int a1 = Receive(ack1, "control");
        int a2 = Receive(ack2, "control");
        if (a1 >= 0 && a2 >= 0)
        {
            LOG(INFO) << "PUB-SUB test successfull";
        }
        else
        {
            LOG(ERROR) << "Failed receiving ack signal: a1 = " << a1 << ", a2 = " << a2;
        }
    }
    else
    {
        LOG(ERROR) << "Failed receiving ready signal: r1 = " << r1 << ", r2 = " << r2;
    }
}

FairMQTestPub::~FairMQTestPub()
{
}

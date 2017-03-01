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

#include "FairMQTestSub.h"
#include "FairMQLogger.h"

FairMQTestSub::FairMQTestSub()
{
}

void FairMQTestSub::Run()
{
    FairMQMessagePtr ready(NewMessage());
    int r1 = Send(ready, "control");
    if (r1 >= 0)
    {
        FairMQMessagePtr msg(NewMessage());
        int d1 = Receive(msg, "data");
        if (d1 >= 0)
        {
            FairMQMessagePtr ack(NewMessage());
            int a1 = Send(ack, "control");
            if (a1 < 0)
            {
                LOG(ERROR) << "Failed sending ack signal: a1 = " << a1;
            }
        }
        else
        {
            LOG(ERROR) << "Failed receiving data: d1 = " << d1;
        }
    }
    else
    {
        LOG(ERROR) << "Failed sending ready signal: r1 = " << r1;
    }
}

FairMQTestSub::~FairMQTestSub()
{
}

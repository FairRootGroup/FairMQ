/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQExampleMTSampler2.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h"

using namespace std;

FairMQExampleMTSampler2::FairMQExampleMTSampler2()
{
}

bool FairMQExampleMTSampler2::ConditionalRun()
{
    FairMQMessagePtr msg(NewMessage(1000));

    // in case of error or transfer interruption, return false to go to IDLE state
    // successfull transfer will return number of bytes transfered (can be 0 if sending an empty message).
    if (Send(msg, "data2") < 0)
    {
        return false;
    }

    return true;
}

FairMQExampleMTSampler2::~FairMQExampleMTSampler2()
{
}

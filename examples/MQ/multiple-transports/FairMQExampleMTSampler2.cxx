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
    : fMaxIterations(0)
    , fNumIterations(0)
{
}

void FairMQExampleMTSampler2::InitTask()
{
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
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

    if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
    {
        LOG(INFO) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
        return false;
    }

    return true;
}

FairMQExampleMTSampler2::~FairMQExampleMTSampler2()
{
}

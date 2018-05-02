/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQExampleMTSampler1.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h"

using namespace std;

FairMQExampleMTSampler1::FairMQExampleMTSampler1()
    : fAckListener()
    , fMaxIterations(0)
    , fNumIterations(0)
{
}

void FairMQExampleMTSampler1::InitTask()
{
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
}

void FairMQExampleMTSampler1::PreRun()
{
    fAckListener = thread(&FairMQExampleMTSampler1::ListenForAcks, this);
}

bool FairMQExampleMTSampler1::ConditionalRun()
{
    // Creates a message using the transport of channel data1
    FairMQMessagePtr msg(NewMessageFor("data1", 0, 1000000));

    // in case of error or transfer interruption, return false to go to IDLE state
    // successfull transfer will return number of bytes transfered (can be 0 if sending an empty message).
    if (Send(msg, "data1") < 0)
    {
        return false;
    }

    if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
    {
        LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
        return false;
    }

    return true;
}

void FairMQExampleMTSampler1::PostRun()
{
    fAckListener.join();
}

void FairMQExampleMTSampler1::ListenForAcks()
{
    uint64_t numAcks = 0;

    while (CheckCurrentState(RUNNING))
    {
        FairMQMessagePtr ack(NewMessageFor("ack", 0));
        if (Receive(ack, "ack") < 0)
        {
            break;
        }
        ++numAcks;
    }

    LOG(info) << "Acknowledged " << numAcks << " messages";
}

FairMQExampleMTSampler1::~FairMQExampleMTSampler1()
{
}

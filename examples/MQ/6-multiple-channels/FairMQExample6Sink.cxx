/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample6Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <memory> // unique_ptr

#include "FairMQExample6Sink.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h"

using namespace std;

FairMQExample6Sink::FairMQExample6Sink()
    : fReceivedData(false)
    , fReceivedBroadcast(false)
    , fMaxIterations(0)
    , fNumIterations(0)
{
    OnData("broadcast", &FairMQExample6Sink::HandleBroadcast);
    OnData("data", &FairMQExample6Sink::HandleData);
}

void FairMQExample6Sink::InitTask()
{
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
}

bool FairMQExample6Sink::HandleBroadcast(FairMQMessagePtr& msg, int /*index*/)
{
    LOG(INFO) << "Received broadcast: \"" << string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";
    fReceivedBroadcast = true;

    return CheckIterations();
}

bool FairMQExample6Sink::HandleData(FairMQMessagePtr& msg, int /*index*/)
{
    LOG(INFO) << "Received message: \"" << string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";
    fReceivedData = true;

    return CheckIterations();
}

bool FairMQExample6Sink::CheckIterations()
{
    if (fMaxIterations > 0)
    {
        if (fReceivedData && fReceivedBroadcast && ++fNumIterations >= fMaxIterations)
        {
            LOG(INFO) << "Configured maximum number of iterations reached & Received messages from both sources. Leaving RUNNING state.";
            return false;
        }
    }

    return true;
}

FairMQExample6Sink::~FairMQExample6Sink()
{
}

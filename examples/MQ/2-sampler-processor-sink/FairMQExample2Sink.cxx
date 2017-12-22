/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample2Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "FairMQExample2Sink.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h" // device->fConfig

using namespace std;

FairMQExample2Sink::FairMQExample2Sink()
    : fMaxIterations(0)
    , fNumIterations(0)
{
    // register a handler for data arriving on "data2" channel
    OnData("data2", &FairMQExample2Sink::HandleData);
}

void FairMQExample2Sink::InitTask()
{
    // Get the fMaxIterations value from the command line options (via fConfig)
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
}

// handler is called whenever a message arrives on "data2", with a reference to the message and a sub-channel index (here 0)
bool FairMQExample2Sink::HandleData(FairMQMessagePtr& msg, int /*index*/)
{
    LOG(info) << "Received: \"" << string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";

    if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
    {
        LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
        return false;
    }

    // return true if want to be called again (otherwise return false go to IDLE state)
    return true;
}

FairMQExample2Sink::~FairMQExample2Sink()
{
}

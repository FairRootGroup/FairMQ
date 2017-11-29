/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample4Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "FairMQExample4Sink.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h" // device->fConfig

#include <stdint.h> // uint64_t

FairMQExample4Sink::FairMQExample4Sink()
    : fMaxIterations(0)
    , fNumIterations(0)
{
    OnData("data", &FairMQExample4Sink::HandleData);
}

void FairMQExample4Sink::InitTask()
{
    // Get the fMaxIterations value from the command line options (via fConfig)
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
}

bool FairMQExample4Sink::HandleData(FairMQMessagePtr& msg, int /*index*/)
{
    LOG(INFO) << "Received message: \"" << *(static_cast<uint64_t*>(msg->GetData())) << "\"";

    if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
    {
        LOG(INFO) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
        return false;
    }

    // return true if want to be called again (otherwise go to IDLE state)
    return true;
}

FairMQExample4Sink::~FairMQExample4Sink()
{
}

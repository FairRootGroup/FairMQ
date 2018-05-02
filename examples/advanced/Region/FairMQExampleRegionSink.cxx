/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExampleRegionSink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "FairMQExampleRegionSink.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h" // device->fConfig

using namespace std;

FairMQExampleRegionSink::FairMQExampleRegionSink()
    : fMaxIterations(0)
    , fNumIterations(0)
{
}

void FairMQExampleRegionSink::InitTask()
{
    // Get the fMaxIterations value from the command line options (via fConfig)
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
}

void FairMQExampleRegionSink::Run()
{
    FairMQChannel& dataInChannel = fChannels.at("data").at(0);

    while (CheckCurrentState(RUNNING))
    {
        FairMQMessagePtr msg(dataInChannel.Transport()->CreateMessage());
        dataInChannel.Receive(msg);
        // void* ptr = msg->GetData();

        if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
        {
            LOG(INFO) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
            break;
        }
    }
}

FairMQExampleRegionSink::~FairMQExampleRegionSink()
{
}

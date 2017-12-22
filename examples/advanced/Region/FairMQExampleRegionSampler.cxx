/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExampleRegionSampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "FairMQExampleRegionSampler.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h" // device->fConfig

#include <thread>
#include <atomic>

using namespace std;

FairMQExampleRegionSampler::FairMQExampleRegionSampler()
    : fMsgSize(10000)
    , fMaxIterations(0)
    , fNumIterations(0)
    , fRegion(nullptr)
    , fNumUnackedMsgs(0)
{
}

void FairMQExampleRegionSampler::InitTask()
{
    fMsgSize = fConfig->GetValue<int>("msg-size");
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");

    fRegion = FairMQUnmanagedRegionPtr(NewUnmanagedRegionFor("data",
                                                             0,
                                                             10000000,
                                                             [this](void* data, size_t size, void* hint) { // callback to be called when message buffers no longer needed by transport
                                                                 --fNumUnackedMsgs;
                                                                 if (fMaxIterations > 0)
                                                                 {
                                                                     LOG(debug) << "Received ack";
                                                                 }
                                                             }
                                                             ));
}

bool FairMQExampleRegionSampler::ConditionalRun()
{
    FairMQMessagePtr msg(NewMessageFor("data", // channel
                                        0, // sub-channel
                                        fRegion, // region
                                        fRegion->GetData(), // ptr within region
                                        fMsgSize, // offset from ptr
                                        nullptr // hint
                                        ));

    if (Send(msg, "data", 0) > 0)
    {
        ++fNumUnackedMsgs;

        if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
        {
            LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
            return false;
        }
    }

    return true;
}

void FairMQExampleRegionSampler::ResetTask()
{
    // if not all messages acknowledged, wait for a bit. But only once, since receiver could be already dead.
    if (fNumUnackedMsgs != 0)
    {
        LOG(debug) << "waiting for all acknowledgements... (" << fNumUnackedMsgs << ")";
        this_thread::sleep_for(chrono::milliseconds(500));
        LOG(debug) << "done, still unacked: " << fNumUnackedMsgs;
    }
    fRegion.reset();
}

FairMQExampleRegionSampler::~FairMQExampleRegionSampler()
{
}

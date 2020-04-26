/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Sampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "Sampler.h"

#include <thread>

using namespace std;

namespace example_region
{

Sampler::Sampler()
    : fMsgSize(10000)
    , fMaxIterations(0)
    , fNumIterations(0)
    , fRegion(nullptr)
    , fNumUnackedMsgs(0)
{
}

void Sampler::InitTask()
{
    fMsgSize = fConfig->GetProperty<int>("msg-size");
    fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");

    fChannels.at("data").at(0).Transport()->SubscribeToRegionEvents([](FairMQRegionInfo info) {
        LOG(warn) << ">>>" << info.event;
        LOG(warn) << "id: " << info.id;
        LOG(warn) << "ptr: " << info.ptr;
        LOG(warn) << "size: " << info.size;
        LOG(warn) << "flags: " << info.flags;
    });

    fRegion = FairMQUnmanagedRegionPtr(NewUnmanagedRegionFor("data",
                                                             0,
                                                             10000000,
                                                             [this](void* /*data*/, size_t /*size*/, void* /*hint*/) { // callback to be called when message buffers no longer needed by transport
                                                                 --fNumUnackedMsgs;
                                                                 if (fMaxIterations > 0) {
                                                                     LOG(debug) << "Received ack";
                                                                 }
                                                             }
                                                             ));
}

bool Sampler::ConditionalRun()
{
    FairMQMessagePtr msg(NewMessageFor("data", // channel
                                        0, // sub-channel
                                        fRegion, // region
                                        fRegion->GetData(), // ptr within region
                                        fMsgSize, // offset from ptr
                                        nullptr // hint
                                        ));

    // static_cast<char*>(fRegion->GetData())[3] = 97;
    // LOG(info) << "check: " << static_cast<char*>(fRegion->GetData())[3];
    // std::this_thread::sleep_for(std::chrono::seconds(1));

    if (Send(msg, "data", 0) > 0) {
        ++fNumUnackedMsgs;

        if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
            LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
            return false;
        }
    }

    return true;
}

void Sampler::ResetTask()
{
    // if not all messages acknowledged, wait for a bit. But only once, since receiver could be already dead.
    if (fNumUnackedMsgs != 0) {
        LOG(debug) << "waiting for all acknowledgements... (" << fNumUnackedMsgs << ")";
        this_thread::sleep_for(chrono::milliseconds(500));
        LOG(debug) << "done, still unacked: " << fNumUnackedMsgs;
    }
    fRegion.reset();
    fChannels.at("data").at(0).Transport()->UnsubscribeFromRegionEvents();
}

Sampler::~Sampler()
{
}

} // namespace example_region

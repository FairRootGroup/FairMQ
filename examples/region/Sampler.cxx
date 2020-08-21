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
    , fLinger(100)
    , fMaxIterations(0)
    , fNumIterations(0)
    , fRegion(nullptr)
    , fNumUnackedMsgs(0)
{}

void Sampler::InitTask()
{
    fMsgSize = fConfig->GetProperty<int>("msg-size");
    fLinger = fConfig->GetProperty<uint32_t>("region-linger");
    fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");

    fChannels.at("data").at(0).Transport()->SubscribeToRegionEvents([](FairMQRegionInfo info) {
        LOG(info) << "Region event: " << info.event
                  << ", managed: " << info.managed
                  << ", id: " << info.id
                  << ", ptr: " << info.ptr
                  << ", size: " << info.size
                  << ", flags: " << info.flags;
    });

    fRegion = FairMQUnmanagedRegionPtr(NewUnmanagedRegionFor("data",
                                                             0,
                                                             10000000,
                                                             [this](const std::vector<fair::mq::RegionBlock>& blocks) { // callback to be called when message buffers no longer needed by transport
                                                                 lock_guard<mutex> lock(fMtx);
                                                                 fNumUnackedMsgs -= blocks.size();

                                                                 if (fMaxIterations > 0) {
                                                                    LOG(info) << "Received " << blocks.size() << " acks";
                                                                 }
                                                             }
                                                             ));
    fRegion->SetLinger(fLinger);
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

    lock_guard<mutex> lock(fMtx);
    if (Send(msg, "data", 0) > 0) {
        if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
            LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
            return false;
        }
    }
    ++fNumUnackedMsgs;

    return true;
}

void Sampler::ResetTask()
{
    // On destruction UnmanagedRegion will try to TODO
    fRegion.reset();
    {
        lock_guard<mutex> lock(fMtx);
        if (fNumUnackedMsgs != 0) {
            LOG(info) << "Done, still not acknowledged: " << fNumUnackedMsgs;
        } else {
            LOG(info) << "All acknowledgements received.";
        }
    }
    fChannels.at("data").at(0).Transport()->UnsubscribeFromRegionEvents();
}

Sampler::~Sampler()
{
}

} // namespace example_region

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "Sink.h"

using namespace std;

namespace example_region
{

Sink::Sink()
    : fMaxIterations(0)
    , fNumIterations(0)
{
}

void Sink::InitTask()
{
    // Get the fMaxIterations value from the command line options (via fConfig)
    fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
    fChannels.at("data").at(0).Transport()->SubscribeToRegionEvents([](FairMQRegionInfo info) {
        LOG(info) << "Region event: " << info.event
                  << ", id: " << info.id
                  << ", ptr: " << info.ptr
                  << ", size: " << info.size
                  << ", flags: " << info.flags;
    });
}

void Sink::Run()
{
    FairMQChannel& dataInChannel = fChannels.at("data").at(0);

    while (!NewStatePending()) {
        FairMQMessagePtr msg(dataInChannel.Transport()->CreateMessage());
        dataInChannel.Receive(msg);

        // void* ptr = msg->GetData();
        // char* cptr = static_cast<char*>(ptr);
        // LOG(info) << "check: " << cptr[3];

        if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
            LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
            break;
        }
    }
}

void Sink::ResetTask()
{
    fChannels.at("data").at(0).Transport()->UnsubscribeFromRegionEvents();
}

Sink::~Sink()
{
}

} // namespace example_region

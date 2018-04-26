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

#include <thread> // this_thread::sleep_for
#include <chrono>

#include "Sampler.h"

using namespace std;

namespace example_copypush
{

Sampler::Sampler()
    : fNumDataChannels(0)
    , fCounter(0)
    , fMaxIterations(0)
    , fNumIterations(0)
{
}

void Sampler::InitTask()
{
    fNumDataChannels = fChannels.at("data").size();
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
}

bool Sampler::ConditionalRun()
{

    // NewSimpleMessage creates a copy of the data and takes care of its destruction (after the transfer takes place).
    // Should only be used for small data because of the cost of an additional copy
    FairMQMessagePtr msg(NewSimpleMessage(fCounter++));

    for (int i = 0; i < fNumDataChannels - 1; ++i)
    {
        FairMQMessagePtr msgCopy(NewMessage());
        msgCopy->Copy(*msg);
        Send(msgCopy, "data", i);
    }
    Send(msg, "data", fNumDataChannels - 1);

    if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
    {
        LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
        return false;
    }

    this_thread::sleep_for(chrono::seconds(1));

    return true;
}

Sampler::~Sampler()
{
}

} // namespace example_copypush

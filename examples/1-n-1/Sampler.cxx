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

namespace example_1_n_1
{

Sampler::Sampler()
    : fText()
    , fMaxIterations(0)
    , fNumIterations(0)
{
}

void Sampler::InitTask()
{
    // Get the fText and fMaxIterations values from the command line options (via fConfig)
    fText = fConfig->GetValue<string>("text");
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
}

bool Sampler::ConditionalRun()
{
    // Initializing message with NewStaticMessage will avoid copy
    // but won't delete the data after the sending is completed.
    FairMQMessagePtr msg(NewStaticMessage(fText));

    LOG(info) << "Sending \"" << fText << "\"";

    // in case of error or transfer interruption, return false to go to IDLE state
    // successfull transfer will return number of bytes transfered (can be 0 if sending an empty message).
    if (Send(msg, "data1") < 0)
    {
        return false;
    }
    else if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
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

} // namespace example_1_n_1

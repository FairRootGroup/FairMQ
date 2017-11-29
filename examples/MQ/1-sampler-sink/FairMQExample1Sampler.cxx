/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample1Sampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <thread> // this_thread::sleep_for
#include <chrono>

#include "FairMQExample1Sampler.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h" // device->fConfig

using namespace std;

FairMQExample1Sampler::FairMQExample1Sampler()
    : fText()
    , fMaxIterations(0)
    , fNumIterations(0)
{
}

void FairMQExample1Sampler::InitTask()
{
    // Get the fText and fMaxIterations values from the command line options (via fConfig)
    fText = fConfig->GetValue<string>("text");
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
}

bool FairMQExample1Sampler::ConditionalRun()
{
    // create a copy of the data with new(), that will be deleted after the transfer is complete
    string* text = new string(fText);

    // create message object with a pointer to the data buffer,
    // its size,
    // custom deletion function (called when transfer is done),
    // and pointer to the object managing the data buffer
    FairMQMessagePtr msg(NewMessage(const_cast<char*>(text->c_str()),
                                    text->length(),
                                    [](void* /*data*/, void* object) { delete static_cast<string*>(object); },
                                    text));

    LOG(INFO) << "Sending \"" << fText << "\"";

    // in case of error or transfer interruption, return false to go to IDLE state
    // successfull transfer will return number of bytes transfered (can be 0 if sending an empty message).
    if (Send(msg, "data") < 0)
    {
        return false;
    }
    else if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
    {
        LOG(INFO) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
        return false;
    }

    this_thread::sleep_for(chrono::seconds(1));

    return true;
}

FairMQExample1Sampler::~FairMQExample1Sampler()
{
}

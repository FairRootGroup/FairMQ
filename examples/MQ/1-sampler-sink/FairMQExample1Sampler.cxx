/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
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
{
}

void FairMQExample1Sampler::InitTask()
{
    // Get the fText value from the command line option (via fConfig)
    fText = fConfig->GetValue<string>("text");
}

bool FairMQExample1Sampler::ConditionalRun()
{
    std::this_thread::sleep_for(std::chrono::seconds(1));

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

    return true;
}

FairMQExample1Sampler::~FairMQExample1Sampler()
{
}

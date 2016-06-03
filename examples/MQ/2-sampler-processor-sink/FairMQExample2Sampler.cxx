/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample2Sampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <thread> // this_thread::sleep_for
#include <chrono>

#include "FairMQExample2Sampler.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h" // device->fConfig

using namespace std;

FairMQExample2Sampler::FairMQExample2Sampler()
    : fText()
{
}

void FairMQExample2Sampler::InitTask()
{
    // Get the fText value from the command line option (via fConfig)
    fText = fConfig->GetValue<string>("text");
}

bool FairMQExample2Sampler::ConditionalRun()
{
    this_thread::sleep_for(chrono::seconds(1));

    // Initializing message with NewStaticMessage will avoid copy
    // but won't delete the data after the sending is completed.
    FairMQMessagePtr msg(NewStaticMessage(fText));

    LOG(INFO) << "Sending \"" << fText << "\"";

    // in case of error or transfer interruption, return false to go to IDLE state
    // successfull transfer will return number of bytes transfered (can be 0 if sending an empty message).
    if (Send(msg, "data1") < 0)
    {
        return false;
    }

    return true;
}

FairMQExample2Sampler::~FairMQExample2Sampler()
{
}

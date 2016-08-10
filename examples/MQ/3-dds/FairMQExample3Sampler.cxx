/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample3Sampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <thread> // this_thread::sleep_for
#include <chrono>

#include "FairMQExample3Sampler.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h" // device->fConfig

using namespace std;

FairMQExample3Sampler::FairMQExample3Sampler()
{
}

bool FairMQExample3Sampler::ConditionalRun()
{
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // NewSimpleMessage creates a copy of the data and takes care of its destruction (after the transfer takes place).
    // Should only be used for small data because of the cost of an additional copy
    FairMQMessagePtr msg(NewSimpleMessage("Data"));

    LOG(INFO) << "Sending \"Data\"";

    // in case of error or transfer interruption, return false to go to IDLE state
    // successfull transfer will return number of bytes transfered (can be 0 if sending an empty message).
    if (Send(msg, "data1") < 0)
    {
        return false;
    }

    return true;
}

FairMQExample3Sampler::~FairMQExample3Sampler()
{
}

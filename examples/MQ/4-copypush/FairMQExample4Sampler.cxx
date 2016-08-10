/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample4Sampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <thread> // this_thread::sleep_for
#include <chrono>

#include "FairMQExample4Sampler.h"
#include "FairMQLogger.h"

FairMQExample4Sampler::FairMQExample4Sampler()
    : fNumDataChannels(0)
    , fCounter(0)
{
}

void FairMQExample4Sampler::InitTask()
{
    fNumDataChannels = fChannels.at("data").size();
}

bool FairMQExample4Sampler::ConditionalRun()
{
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // NewSimpleMessage creates a copy of the data and takes care of its destruction (after the transfer takes place).
    // Should only be used for small data because of the cost of an additional copy
    FairMQMessagePtr msg(NewSimpleMessage(fCounter++));

    for (int i = 0; i < fNumDataChannels - 1; ++i)
    {
        FairMQMessagePtr msgCopy(NewMessage());
        msgCopy->Copy(msg);
        Send(msgCopy, "data", i);
    }
    Send(msg, "data", fNumDataChannels - 1);
    return true;
}

FairMQExample4Sampler::~FairMQExample4Sampler()
{
}

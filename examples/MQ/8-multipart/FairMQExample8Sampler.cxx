/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample8Sampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <thread> // this_thread::sleep_for
#include <chrono>

#include "FairMQExample8Sampler.h"
#include "FairMQEx8Header.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h"

using namespace std;

FairMQExample8Sampler::FairMQExample8Sampler()
    : fMaxIterations(5)
    , fNumIterations(0)
{
}

void FairMQExample8Sampler::InitTask()
{
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
}

bool FairMQExample8Sampler::ConditionalRun()
{
    Ex8Header header;
    header.stopFlag = 0;

    // Set stopFlag to 1 for last message.
    if (fMaxIterations > 0 && fNumIterations == fMaxIterations - 1)
    {
        header.stopFlag = 1;
    }
    LOG(INFO) << "Sending header with stopFlag: " << header.stopFlag;

    FairMQParts parts;

    // NewSimpleMessage creates a copy of the data and takes care of its destruction (after the transfer takes place).
    // Should only be used for small data because of the cost of an additional copy
    parts.AddPart(NewSimpleMessage(header));
    parts.AddPart(NewMessage(1000));

    LOG(INFO) << "Sending body of size: " << parts.At(1)->GetSize();

    Send(parts, "data-out");

    // Go out of the sending loop if the stopFlag was sent.
    if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
    {
        LOG(INFO) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
        return false;
    }

    // Wait a second to keep the output readable.
    this_thread::sleep_for(chrono::seconds(1));

    return true;
}

FairMQExample8Sampler::~FairMQExample8Sampler()
{
}

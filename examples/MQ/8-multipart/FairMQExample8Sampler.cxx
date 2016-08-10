/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
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

using namespace std;

FairMQExample8Sampler::FairMQExample8Sampler()
    : fCounter(0)
{
}

bool FairMQExample8Sampler::ConditionalRun()
{
    // Wait a second to keep the output readable.
    this_thread::sleep_for(chrono::seconds(1));

    Ex8Header header;
    // Set stopFlag to 1 for the first 4 messages, and to 0 for the 5th.
    fCounter < 5 ? header.stopFlag = 0 : header.stopFlag = 1;
    LOG(INFO) << "Sending header with stopFlag: " << header.stopFlag;

    FairMQParts parts;

    // NewSimpleMessage creates a copy of the data and takes care of its destruction (after the transfer takes place).
    // Should only be used for small data because of the cost of an additional copy
    parts.AddPart(NewSimpleMessage(header));
    parts.AddPart(NewMessage(1000));

    LOG(INFO) << "Sending body of size: " << parts.At(1)->GetSize();

    Send(parts, "data-out");

    // Go out of the sending loop if the stopFlag was sent.
    if (fCounter++ == 5)
    {
        return false;
    }

    return true;
}

FairMQExample8Sampler::~FairMQExample8Sampler()
{
}

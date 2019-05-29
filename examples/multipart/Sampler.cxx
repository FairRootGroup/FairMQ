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
#include "Header.h"

using namespace std;

namespace example_multipart
{

Sampler::Sampler()
    : fMaxIterations(5)
    , fNumIterations(0)
{
}

void Sampler::InitTask()
{
    fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
}

bool Sampler::ConditionalRun()
{
    Header header;
    header.stopFlag = 0;

    // Set stopFlag to 1 for last message.
    if (fMaxIterations > 0 && fNumIterations == fMaxIterations - 1)
    {
        header.stopFlag = 1;
    }
    LOG(info) << "Sending header with stopFlag: " << header.stopFlag;

    FairMQParts parts;

    // NewSimpleMessage creates a copy of the data and takes care of its destruction (after the transfer takes place).
    // Should only be used for small data because of the cost of an additional copy
    parts.AddPart(NewSimpleMessage(header));
    parts.AddPart(NewMessage(1000));

    // create more data parts, testing the FairMQParts in-place constructor
    FairMQParts auxData{ NewMessage(500), NewMessage(600), NewMessage(700) };
    assert(auxData.Size() == 3);
    parts.AddPart(std::move(auxData));
    assert(auxData.Size() == 0);
    assert(parts.Size() == 5);

    LOG(info) << "Sending body of size: " << parts.At(1)->GetSize();

    Send(parts, "data");

    // Go out of the sending loop if the stopFlag was sent.
    if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
    {
        LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
        return false;
    }

    // Wait a second to keep the output readable.
    this_thread::sleep_for(chrono::seconds(1));

    return true;
}

Sampler::~Sampler()
{
}

} // namespace example_multipart

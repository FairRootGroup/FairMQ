/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Sampler2.h"

using namespace std;

namespace example_multiple_transports
{

Sampler2::Sampler2()
    : fMaxIterations(0)
    , fNumIterations(0)
{
}

void Sampler2::InitTask()
{
    fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
}

bool Sampler2::ConditionalRun()
{
    FairMQMessagePtr msg(NewMessage(1000));

    // in case of error or transfer interruption, return false to go to IDLE state
    // successfull transfer will return number of bytes transfered (can be 0 if sending an empty message).
    if (Send(msg, "data2") < 0)
    {
        return false;
    }

    if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
    {
        LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
        return false;
    }

    return true;
}

Sampler2::~Sampler2()
{
}

} // namespace example_multiple_transports

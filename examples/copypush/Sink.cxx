/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "Sink.h"

namespace example_copypush
{

Sink::Sink()
    : fMaxIterations(0)
    , fNumIterations(0)
{
    OnData("data", &Sink::HandleData);
}

void Sink::InitTask()
{
    // Get the fMaxIterations value from the command line options (via fConfig)
    fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
}

bool Sink::HandleData(FairMQMessagePtr& msg, int /*index*/)
{
    LOG(info) << "Received message: \"" << *(static_cast<uint64_t*>(msg->GetData())) << "\"";

    if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
    {
        LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
        return false;
    }

    // return true if want to be called again (otherwise go to IDLE state)
    return true;
}

Sink::~Sink()
{
}

} // namespace example_copypush

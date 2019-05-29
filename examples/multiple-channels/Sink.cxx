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

using namespace std;

namespace example_multiple_channels
{

Sink::Sink()
    : fReceivedData(false)
    , fReceivedBroadcast(false)
    , fMaxIterations(0)
    , fNumIterations(0)
{
    OnData("broadcast", &Sink::HandleBroadcast);
    OnData("data", &Sink::HandleData);
}

void Sink::InitTask()
{
    fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
}

bool Sink::HandleBroadcast(FairMQMessagePtr& msg, int /*index*/)
{
    LOG(info) << "Received broadcast: \"" << string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";
    fReceivedBroadcast = true;

    return CheckIterations();
}

bool Sink::HandleData(FairMQMessagePtr& msg, int /*index*/)
{
    LOG(info) << "Received message: \"" << string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";
    fReceivedData = true;

    return CheckIterations();
}

bool Sink::CheckIterations()
{
    if (fMaxIterations > 0)
    {
        if (fReceivedData && fReceivedBroadcast && ++fNumIterations >= fMaxIterations)
        {
            LOG(info) << "Configured maximum number of iterations reached & Received messages from both sources. Leaving RUNNING state.";
            return false;
        }
    }

    return true;
}

Sink::~Sink()
{
}

}

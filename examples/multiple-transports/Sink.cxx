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

namespace example_multiple_transports
{

Sink::Sink()
    : fMaxIterations(0)
    , fNumIterations1(0)
    , fNumIterations2(0)
{
    // register a handler for data arriving on "data" channel
    OnData("data1", &Sink::HandleData1);
    OnData("data2", &Sink::HandleData2);
}

void Sink::InitTask()
{
    fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
}

// handler is called whenever a message arrives on "data", with a reference to the message and a sub-channel index (here 0)
bool Sink::HandleData1(FairMQMessagePtr& /*msg*/, int /*index*/)
{
    fNumIterations1++;
    // Creates a message using the transport of channel ack
    FairMQMessagePtr ack(NewMessageFor("ack", 0));
    if (Send(ack, "ack") < 0)
    {
        return false;
    }

    // return true if want to be called again (otherwise go to IDLE state)
    return CheckIterations();
}

// handler is called whenever a message arrives on "data", with a reference to the message and a sub-channel index (here 0)
bool Sink::HandleData2(FairMQMessagePtr& /*msg*/, int /*index*/)
{
    fNumIterations2++;
    // return true if want to be called again (otherwise go to IDLE state)
    return CheckIterations();
}

bool Sink::CheckIterations()
{
    if (fMaxIterations > 0)
    {
        if (fNumIterations1 >= fMaxIterations && fNumIterations2 >= fMaxIterations)
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

} // namespace example_multiple_transports

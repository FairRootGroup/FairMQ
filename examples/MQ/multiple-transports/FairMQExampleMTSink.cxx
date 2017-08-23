/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExampleMTSink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "FairMQExampleMTSink.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h"

using namespace std;

FairMQExampleMTSink::FairMQExampleMTSink()
    : fMaxIterations(0)
    , fNumIterations1(0)
    , fNumIterations2(0)
    , fReceived1(false)
    , fReceived2(false)
{
    // register a handler for data arriving on "data" channel
    OnData("data1", &FairMQExampleMTSink::HandleData1);
    OnData("data2", &FairMQExampleMTSink::HandleData2);
}

void FairMQExampleMTSink::InitTask()
{
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
}

// handler is called whenever a message arrives on "data", with a reference to the message and a sub-channel index (here 0)
bool FairMQExampleMTSink::HandleData1(FairMQMessagePtr& /*msg*/, int /*index*/)
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
bool FairMQExampleMTSink::HandleData2(FairMQMessagePtr& /*msg*/, int /*index*/)
{
    fNumIterations2++;
    // return true if want to be called again (otherwise go to IDLE state)
    return CheckIterations();
}

bool FairMQExampleMTSink::CheckIterations()
{
    if (fMaxIterations > 0)
    {
        if (fNumIterations1 >= fMaxIterations && fNumIterations2 >= fMaxIterations)
        {
            LOG(INFO) << "Configured maximum number of iterations reached & Received messages from both sources. Leaving RUNNING state.";
            return false;
        }
    }

    return true;
}

FairMQExampleMTSink::~FairMQExampleMTSink()
{
}

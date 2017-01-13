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

using namespace std;

FairMQExampleMTSink::FairMQExampleMTSink()
{
    // register a handler for data arriving on "data" channel
    OnData("data1", &FairMQExampleMTSink::HandleData1);
    OnData("data2", &FairMQExampleMTSink::HandleData2);
}

// handler is called whenever a message arrives on "data", with a reference to the message and a sub-channel index (here 0)
bool FairMQExampleMTSink::HandleData1(FairMQMessagePtr& /*msg*/, int /*index*/)
{
    // Creates a message using the transport of channel ack
    FairMQMessagePtr ack(NewMessageFor("ack", 0));
    if (Send(ack, "ack") < 0)
    {
        return false;
    }

    // return true if want to be called again (otherwise go to IDLE state)
    return true;
}

// handler is called whenever a message arrives on "data", with a reference to the message and a sub-channel index (here 0)
bool FairMQExampleMTSink::HandleData2(FairMQMessagePtr& /*msg*/, int /*index*/)
{
    // return true if want to be called again (otherwise go to IDLE state)
    return true;
}

FairMQExampleMTSink::~FairMQExampleMTSink()
{
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample1Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "FairMQExample1Sink.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample1Sink::FairMQExample1Sink()
{
    // register a handler for data arriving on "data" channel
    OnData("data", &FairMQExample1Sink::HandleData);
}

// handler is called whenever a message arrives on "data", with a reference to the message and a sub-channel index (here 0)
bool FairMQExample1Sink::HandleData(FairMQMessagePtr& msg, int /*index*/)
{
    LOG(INFO) << "Received: \"" << string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";

    // return true if want to be called again (otherwise go to IDLE state)
    return true;
}

FairMQExample1Sink::~FairMQExample1Sink()
{
}

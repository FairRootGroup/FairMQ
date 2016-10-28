/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample6Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <memory> // unique_ptr

#include "FairMQExample6Sink.h"
#include "FairMQPoller.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample6Sink::FairMQExample6Sink()
{
    OnData("broadcast", &FairMQExample6Sink::HandleBroadcast);
    OnData("data", &FairMQExample6Sink::HandleData);
}

bool FairMQExample6Sink::HandleBroadcast(FairMQMessagePtr& msg, int /*index*/)
{
    LOG(INFO) << "Received broadcast: \"" << string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";

    return true;
}

bool FairMQExample6Sink::HandleData(FairMQMessagePtr& msg, int /*index*/)
{
    LOG(INFO) << "Received message: \"" << string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";

    return true;
}

FairMQExample6Sink::~FairMQExample6Sink()
{
}

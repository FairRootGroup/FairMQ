/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample4Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "FairMQExample4Sink.h"
#include "FairMQLogger.h"

#include <stdint.h> // uint64_t

FairMQExample4Sink::FairMQExample4Sink()
{
    OnData("data", &FairMQExample4Sink::HandleData);
}

bool FairMQExample4Sink::HandleData(FairMQMessagePtr& msg, int /*index*/)
{
    LOG(INFO) << "Received message: \"" << *(static_cast<uint64_t*>(msg->GetData())) << "\"";

    // return true if want to be called again (otherwise go to IDLE state)
    return true;
}

FairMQExample4Sink::~FairMQExample4Sink()
{
}

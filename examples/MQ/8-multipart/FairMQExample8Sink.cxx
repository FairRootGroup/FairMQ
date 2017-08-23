/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample8Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "FairMQExample8Sink.h"
#include "FairMQEx8Header.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample8Sink::FairMQExample8Sink()
{
    OnData("data-in", &FairMQExample8Sink::HandleData);
}

bool FairMQExample8Sink::HandleData(FairMQParts& parts, int /*index*/)
{
    Ex8Header header;
    header.stopFlag = (static_cast<Ex8Header*>(parts.At(0)->GetData()))->stopFlag;

    LOG(INFO) << "Received message with " << parts.Size() << " parts";

    LOG(INFO) << "Received header with stopFlag: " << header.stopFlag;
    LOG(INFO) << "Received body of size: " << parts.At(1)->GetSize();

    if (header.stopFlag == 1)
    {
        LOG(INFO) << "stopFlag is 1, going IDLE";
        return false;
    }

    return true;
}

FairMQExample8Sink::~FairMQExample8Sink()
{
}

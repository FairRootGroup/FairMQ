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

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExample8Sink.h"
#include "FairMQLogger.h"

using namespace std;

struct Ex8Header {
  int32_t stopFlag;
};

FairMQExample8Sink::FairMQExample8Sink()
{
}

void FairMQExample8Sink::Run()
{
    while (CheckCurrentState(RUNNING))
    {
        FairMQParts parts;

        if (Receive(parts, "data-in") >= 0)
        {
            Ex8Header header;
            header.stopFlag = (static_cast<Ex8Header*>(parts.At(0).GetData()))->stopFlag;
            LOG(INFO) << "Received header with stopFlag: " << header.stopFlag;
            LOG(INFO) << "Received body of size: " << parts.At(1).GetSize();
            if (header.stopFlag == 1)
            {
                LOG(INFO) << "Flag is 0, exiting Run()";
                break;
            }
        }
    }
}

FairMQExample8Sink::~FairMQExample8Sink()
{
}

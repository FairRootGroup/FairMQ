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
        unique_ptr<FairMQMessage> headerPart(fTransportFactory->CreateMessage());
        unique_ptr<FairMQMessage> bodyPart(fTransportFactory->CreateMessage());

        if (fChannels.at("data-in").at(0).Receive(headerPart) >= 0)
        {
            if (fChannels.at("data-in").at(0).Receive(bodyPart) >= 0)
            {
                Ex8Header header;
                header.stopFlag = (static_cast<Ex8Header*>(headerPart->GetData()))->stopFlag;
                LOG(INFO) << "Received header with stopFlag: " << header.stopFlag;
                if (header.stopFlag == 1)
                {
                    LOG(INFO) << "Flag is 0, exiting Run()";
                    break;
                }
            }
        }
    }
}

FairMQExample8Sink::~FairMQExample8Sink()
{
}

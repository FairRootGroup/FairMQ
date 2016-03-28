/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample8Sampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExample8Sampler.h"
#include "FairMQLogger.h"

using namespace std;

struct Ex8Header {
  int32_t stopFlag;
};

FairMQExample8Sampler::FairMQExample8Sampler()
{
}

void FairMQExample8Sampler::Run()
{
    int counter = 0;

    // Check if we are still in the RUNNING state.
    while (CheckCurrentState(RUNNING))
    {
        Ex8Header* header = new Ex8Header;
        // Set stopFlag to 1 for the first 4 messages, and to 0 for the 5th.
        counter < 5 ? header->stopFlag = 0 : header->stopFlag = 1;
        LOG(INFO) << "Sending header with stopFlag: " << header->stopFlag;

        FairMQParts parts;
        LOG(INFO) << "Sending body of size: " << parts.At(1)->GetSize();

        Send(parts, "data-out");

        // Go out of the sending loop if the stopFlag was sent.
        if (counter == 5)
        {
            break;
        }

        counter++;
        // Wait a second to keep the output readable.
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    }

    LOG(INFO) << "Going out of RUNNING state.";
}

FairMQExample8Sampler::~FairMQExample8Sampler()
{
}

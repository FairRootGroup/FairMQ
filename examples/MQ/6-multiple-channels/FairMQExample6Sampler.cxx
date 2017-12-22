/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample6Sampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <memory> // unique_ptr
#include <thread> // this_thread::sleep_for
#include <chrono>

#include "FairMQExample6Sampler.h"
#include "FairMQPoller.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h"

using namespace std;

FairMQExample6Sampler::FairMQExample6Sampler()
    : fText()
    , fMaxIterations(0)
    , fNumIterations(0)
{
}

void FairMQExample6Sampler::InitTask()
{
    fText = fConfig->GetValue<string>("text");
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
}

void FairMQExample6Sampler::Run()
{
    FairMQPollerPtr poller(NewPoller("data", "broadcast"));

    while (CheckCurrentState(RUNNING))
    {
        poller->Poll(100);

        if (poller->CheckInput("broadcast", 0))
        {
            FairMQMessagePtr msg(NewMessage());

            if (Receive(msg, "broadcast") > 0)
            {
                LOG(info) << "Received broadcast: \"" << string(static_cast<char*>(msg->GetData()), msg->GetSize()) << "\"";
            }
        }

        if (poller->CheckOutput("data", 0))
        {
            this_thread::sleep_for(chrono::seconds(1));

            FairMQMessagePtr msg(NewSimpleMessage(fText));

            if (Send(msg, "data") > 0)
            {
                LOG(info) << "Sent \"" << fText << "\"";

                if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
                {
                    LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
                    break;
                }
            }
        }
    }
}

FairMQExample6Sampler::~FairMQExample6Sampler()
{
}

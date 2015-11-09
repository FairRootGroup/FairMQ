/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample4Sampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <memory> // unique_ptr

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExample4Sampler.h"
#include "FairMQLogger.h"

FairMQExample4Sampler::FairMQExample4Sampler()
{
}

void FairMQExample4Sampler::Run()
{
    uint64_t counter = 0;

    while (CheckCurrentState(RUNNING))
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        uint64_t* number = new uint64_t(counter);

        std::unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage(number, sizeof(uint64_t)));

        LOG(INFO) << "Sending \"" << counter << "\"";

        if (fChannels.at("data-out").size() > 1)
        {
            for (int i = 1; i < fChannels.at("data-out").size(); ++i)
            {
                std::unique_ptr<FairMQMessage> msgCopy(fTransportFactory->CreateMessage());
                msgCopy->Copy(msg);
                fChannels.at("data-out").at(i).Send(msgCopy);
            }
            fChannels.at("data-out").at(0).Send(msg);
        }
        else
        {
            fChannels.at("data-out").at(0).Send(msg);
        }

        ++counter;
    }
}

FairMQExample4Sampler::~FairMQExample4Sampler()
{
}

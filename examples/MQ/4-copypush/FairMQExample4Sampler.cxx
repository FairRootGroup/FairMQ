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

        std::unique_ptr<FairMQMessage> msg(NewMessage(number, // data pointer
                                                      sizeof(uint64_t), // data size
                                                      [](void* data, void* /*hint*/){ delete static_cast<uint64_t*>(data); } // callback to deallocate after the transfer
                                                      ));

        LOG(INFO) << "Sending \"" << counter << "\"";

        if (fChannels.at("data").size() > 1)
        {
            for (unsigned int i = 1; i < fChannels.at("data").size(); ++i)
            {
                std::unique_ptr<FairMQMessage> msgCopy(NewMessage());
                msgCopy->Copy(msg);
                Send(msgCopy, "data", i);
            }
            Send(msg, "data");
        }
        else
        {
            Send(msg, "data");
        }

        ++counter;
    }
}

FairMQExample4Sampler::~FairMQExample4Sampler()
{
}

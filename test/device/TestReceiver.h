/********************************************************************************
 * Copyright (C) 2015-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_TESTRECEIVER_H
#define FAIR_MQ_TEST_TESTRECEIVER_H

#include <fairmq/Device.h>
#include <fairlogger/Logger.h>

#include <string>
#include <thread>

namespace fair::mq::test
{

class Receiver : public Device
{
  public:
    Receiver(const std::string& channelName)
        : fChannelName(channelName)
    {}

  protected:
    auto Init() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        auto msg = NewMessage();
        if (Receive(msg, fChannelName) >= 0) {
            LOG(info) << "received empty message";
        } else {
            LOG(error) << "fair::mq::test::Receiver::Run(): Receive(msg, fChannelName) < 0";
        }
    };

    std::string fChannelName;
};

} // namespace fair::mq::test

#endif // FAIR_MQ_TEST_TESTRECEIVER_H

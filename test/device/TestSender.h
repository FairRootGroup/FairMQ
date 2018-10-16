/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_TESTSENDER_H
#define FAIR_MQ_TEST_TESTSENDER_H

#include <FairMQDevice.h>
#include <FairMQLogger.h>

#include <string>
#include <thread>

namespace fair
{
namespace mq
{
namespace test
{

class Sender : public FairMQDevice
{
  public:
    Sender(const std::string& channelName)
        : fChannelName(channelName)
    {}

  protected:
    auto Init() -> void override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    auto Run() -> void override
    {
        auto msg = FairMQMessagePtr{NewMessage()};
        if (Send(msg, fChannelName) >= 0)
        {
            LOG(info) << "sent empty message";
        }
        else
        {
            LOG(error) << "fair::mq::test::Sender::Run(): Send(msg, fChannelName) < 0";
        }
    };

    std::string fChannelName;
};

} // namespace test
} // namespace mq
} // namespace fair

#endif // FAIR_MQ_TEST_TESTSENDER_H

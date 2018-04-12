/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "TestSender.h"
#include "TestReceiver.h"

#include <gtest/gtest.h>

#include <string>
#include <future> // std::async, std::future

namespace
{

using namespace std;

class MultipleDevices : public ::testing::Test {
  public:
    MultipleDevices()
    {}

    bool TestFirst()
    {
        fair::mq::test::Sender sender("data");

        sender.SetTransport("zeromq");

        FairMQChannel channel("push", "connect", "ipc://multiple-devices-test");
        channel.UpdateRateLogging(0);
        sender.fChannels["data"].push_back(channel);

        sender.ChangeState("INIT_DEVICE");
        sender.WaitForEndOfState("INIT_DEVICE");
        sender.ChangeState("INIT_TASK");
        sender.WaitForEndOfState("INIT_TASK");

        sender.ChangeState("RUN");
        sender.WaitForEndOfState("RUN");

        sender.ChangeState("RESET_TASK");
        sender.WaitForEndOfState("RESET_TASK");
        sender.ChangeState("RESET_DEVICE");
        sender.WaitForEndOfState("RESET_DEVICE");

        sender.ChangeState("END");

        return true;
    }

    bool TestSecond()
    {
        fair::mq::test::Receiver receiver("data");

        receiver.SetTransport("zeromq");

        FairMQChannel channel("pull", "bind", "ipc://multiple-devices-test");
        channel.UpdateRateLogging(0);
        receiver.fChannels["data"].push_back(channel);

        receiver.ChangeState("INIT_DEVICE");
        receiver.WaitForEndOfState("INIT_DEVICE");
        receiver.ChangeState("INIT_TASK");
        receiver.WaitForEndOfState("INIT_TASK");

        receiver.ChangeState("RUN");
        receiver.WaitForEndOfState("RUN");

        receiver.ChangeState("RESET_TASK");
        receiver.WaitForEndOfState("RESET_TASK");
        receiver.ChangeState("RESET_DEVICE");
        receiver.WaitForEndOfState("RESET_DEVICE");

        receiver.ChangeState("END");

        return true;
    }
};

TEST_F(MultipleDevices, TwoInSameProcess)
{
    std::future<bool> fut1 = std::async(std::launch::async, &MultipleDevices::TestFirst, this);
    std::future<bool> fut2 = std::async(std::launch::async, &MultipleDevices::TestSecond, this);

    bool first = fut1.get();
    bool second = fut2.get();

    ASSERT_EQ(first, true);
    ASSERT_EQ(second, true);
}

} // namespace

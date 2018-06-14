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
#include <thread>
#include <future> // std::async, std::future

namespace
{

using namespace std;

void control(FairMQDevice& device)
{
    device.ChangeState("INIT_DEVICE");
    device.WaitForEndOfState("INIT_DEVICE");
    device.ChangeState("INIT_TASK");
    device.WaitForEndOfState("INIT_TASK");

    device.ChangeState("RUN");
    device.WaitForEndOfState("RUN");

    device.ChangeState("RESET_TASK");
    device.WaitForEndOfState("RESET_TASK");
    device.ChangeState("RESET_DEVICE");
    device.WaitForEndOfState("RESET_DEVICE");

    device.ChangeState("END");
}

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

        thread t(control, std::ref(sender));

        sender.RunStateMachine();

        if (t.joinable())
        {
            t.join();
        }

        return true;
    }

    bool TestSecond()
    {
        fair::mq::test::Receiver receiver("data");

        receiver.SetTransport("zeromq");

        FairMQChannel channel("pull", "bind", "ipc://multiple-devices-test");
        channel.UpdateRateLogging(0);
        receiver.fChannels["data"].push_back(channel);

        thread t(control, std::ref(receiver));

        receiver.RunStateMachine();

        if (t.joinable())
        {
            t.join();
        }

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

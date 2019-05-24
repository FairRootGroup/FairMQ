/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
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
    device.ChangeState(fair::mq::Transition::InitDevice);
    device.WaitForState(fair::mq::State::InitializingDevice);
    device.ChangeState(fair::mq::Transition::CompleteInit);
    device.WaitForState(fair::mq::State::Initialized);
    device.ChangeState(fair::mq::Transition::Bind);
    device.WaitForState(fair::mq::State::Bound);
    device.ChangeState(fair::mq::Transition::Connect);
    device.WaitForState(fair::mq::State::DeviceReady);
    device.ChangeState(fair::mq::Transition::InitTask);
    device.WaitForState(fair::mq::State::Ready);

    device.ChangeState(fair::mq::Transition::Run);
    device.WaitForState(fair::mq::State::Ready);

    device.ChangeState(fair::mq::Transition::ResetTask);
    device.WaitForState(fair::mq::State::DeviceReady);
    device.ChangeState(fair::mq::Transition::ResetDevice);
    device.WaitForState(fair::mq::State::Idle);

    device.ChangeState(fair::mq::Transition::End);
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
        sender.AddChannel("data", std::move(channel));

        thread t(control, std::ref(sender));

        sender.RunStateMachine();

        if (t.joinable()) {
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
        receiver.AddChannel("data", std::move(channel));

        thread t(control, std::ref(receiver));

        receiver.RunStateMachine();

        if (t.joinable()) {
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

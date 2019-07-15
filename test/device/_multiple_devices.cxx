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
using namespace fair::mq;

void control(FairMQDevice& device)
{
    thread t([&] {
        device.ChangeState(Transition::InitDevice);
        device.WaitForState(State::InitializingDevice);
        device.ChangeState(Transition::CompleteInit);
        device.WaitForState(State::Initialized);
        device.ChangeState(Transition::Bind);
        device.WaitForState(State::Bound);
        device.ChangeState(Transition::Connect);
        device.WaitForState(State::DeviceReady);
        device.ChangeState(Transition::InitTask);
        device.WaitForState(State::Ready);

        device.ChangeState(Transition::Run);
        device.WaitForState(State::Ready);

        device.ChangeState(Transition::ResetTask);
        device.WaitForState(State::DeviceReady);
        device.ChangeState(Transition::ResetDevice);
        device.WaitForState(State::Idle);

        device.ChangeState(Transition::End);
    });

    device.RunStateMachine();

    if (t.joinable()) {
        t.join();
    }
}

class MultipleDevices : public ::testing::Test {
  public:
    MultipleDevices() {}

    bool TestFirst()
    {
        test::Sender sender("data");
        sender.SetTransport("zeromq");

        FairMQChannel channel("push", "connect", "ipc://multiple-devices-test");
        channel.UpdateRateLogging(0);
        sender.AddChannel("data", std::move(channel));

        control(sender);
        return true;
    }

    bool TestSecond()
    {
        test::Receiver receiver("data");
        receiver.SetTransport("zeromq");

        FairMQChannel channel("pull", "bind", "ipc://multiple-devices-test");
        channel.UpdateRateLogging(0);
        receiver.AddChannel("data", std::move(channel));

        control(receiver);
        return true;
    }
};

TEST_F(MultipleDevices, TwoInSameProcess)
{
    future<bool> fut1 = async(launch::async, &MultipleDevices::TestFirst, this);
    future<bool> fut2 = async(launch::async, &MultipleDevices::TestSecond, this);

    bool first = fut1.get();
    bool second = fut2.get();

    ASSERT_EQ(first, true);
    ASSERT_EQ(second, true);
}

} // namespace

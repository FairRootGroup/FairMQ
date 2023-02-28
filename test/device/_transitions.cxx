/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairlogger/Logger.h>

#include <gtest/gtest.h>

#include <vector>
#include <thread>

namespace
{

using namespace std;
using namespace fair::mq;

class SlowDevice : public Device
{
  public:
    SlowDevice() = default;

  protected:
    void Init() override
    {
        this_thread::sleep_for(chrono::milliseconds(100));
    }
};

void transitionTo(const vector<State>& states, int numExpectedStates)
{
    Device device;

    thread t([&] {
        for (const auto& s : states) {
            device.TransitionTo(s);
        }
    });

    int numStates = 0;

    device.SubscribeToStateChange("testRunner", [&numStates](State /* state */) {
        numStates++;
    });

    device.RunStateMachine();

    if (t.joinable()) {
        t.join();
    }

    LOG(info) << "expected " << numExpectedStates << ", encountered " << numStates << " states";
    EXPECT_EQ(numStates, numExpectedStates);
}

TEST(Transitions, TransitionTo)
{
    transitionTo({State::Exiting}, 2);
    transitionTo({State::InitializingDevice, State::Initialized, State::Exiting}, 6);
    transitionTo({State::Initialized, State::Exiting}, 6);
    transitionTo({State::DeviceReady, State::Bound, State::Running, State::Exiting}, 24);
    transitionTo({State::Ready, State::Exiting}, 14);
    transitionTo({State::Running, State::Exiting}, 16);
}

TEST(Transitions, ConcurrentTransitionTos)
{
    fair::Logger::SetConsoleSeverity("debug");
    SlowDevice slowDevice;

    vector<State> states({State::Ready, State::Exiting});

    thread t1([&] {
        for (const auto& s : states) {
            slowDevice.TransitionTo(s);
        }
    });

    thread t2([&] {
        this_thread::sleep_for(chrono::milliseconds(50));
        ASSERT_THROW(slowDevice.TransitionTo(State::Exiting), OngoingTransition);
    });

    slowDevice.RunStateMachine();

    if (t1.joinable()) {
        t1.join();
    }

    if (t2.joinable()) {
        t2.join();
    }
}

TEST(Transitions, InvalidChangeState)
{
    Device device;
    thread t([&] { device.RunStateMachine(); });

    ASSERT_FALSE(device.ChangeState(Transition::Connect));

    ASSERT_TRUE(device.ChangeState(Transition::End));
    if (t.joinable()) { t.join(); }
}

TEST(Transitions, InvalidChangeStateOrThrow)
{
    Device device;
    thread t([&] { device.RunStateMachine(); });

    ASSERT_THROW(device.ChangeStateOrThrow(Transition::Connect), std::system_error);

    ASSERT_NO_THROW(device.ChangeStateOrThrow(Transition::End));
    if (t.joinable()) { t.join(); }
}

} // namespace

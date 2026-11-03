/********************************************************************************
 * Copyright (C) 2017-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Fixture.h"
#include <array>
#include <condition_variable>
#include <fairmq/Tools.h>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

using namespace std;
using fair::mq::test::PluginServices;
using DeviceState = fair::mq::PluginServices::DeviceState;
using DeviceStateTransition = fair::mq::PluginServices::DeviceStateTransition;

TEST_F(PluginServices, OnlySingleController)
{
    ASSERT_NO_THROW(mServices.TakeDeviceControl("foo"));
    ASSERT_NO_THROW(mServices.TakeDeviceControl("foo")); // noop
    ASSERT_THROW( // no control for bar
        mServices.ChangeDeviceState("bar", DeviceStateTransition::InitDevice),
        fair::mq::PluginServices::DeviceControlError
    );
    ASSERT_NO_THROW(mServices.ReleaseDeviceControl("bar"));

    ASSERT_NO_THROW(mServices.ReleaseDeviceControl("foo"));
    ASSERT_FALSE(mServices.GetDeviceController());
    // take control implicitely
    ASSERT_NO_THROW(mServices.TakeDeviceControl("foo"));
    ASSERT_NO_THROW(mServices.ChangeDeviceState("foo", DeviceStateTransition::InitDevice));
    EXPECT_EQ(mServices.GetDeviceController(), string{"foo"});

    mDevice.WaitForState(fair::mq::State::InitializingDevice);
    mServices.ChangeDeviceState("foo", DeviceStateTransition::CompleteInit);
    mDevice.WaitForState(fair::mq::State::Initialized);
    mServices.ChangeDeviceState("foo", DeviceStateTransition::Bind);
    mDevice.WaitForState(fair::mq::State::Bound);
    mServices.ChangeDeviceState("foo", DeviceStateTransition::Connect);

    // park device
    mDevice.WaitForState(fair::mq::State::DeviceReady);
    mServices.ChangeDeviceState("foo", DeviceStateTransition::ResetDevice);
    mDevice.WaitForState(fair::mq::State::Idle);
    mServices.ChangeDeviceState("foo", DeviceStateTransition::End);
    mDevice.WaitForState(fair::mq::State::Exiting);
}

TEST_F(PluginServices, Control)
{
    ASSERT_EQ(mServices.GetCurrentDeviceState(), DeviceState::Idle);
    ASSERT_NO_THROW(mServices.TakeDeviceControl("foo"));
    ASSERT_NO_THROW(mServices.ChangeDeviceState("foo", DeviceStateTransition::InitDevice));

    DeviceState nextState;
    condition_variable cv;
    mutex cv_m;
    mServices.SubscribeToDeviceStateChange("test", [&](DeviceState newState){
        ASSERT_NE(newState, DeviceState::ResettingDevice); // check UnsubscribeFromDeviceStateChange

        lock_guard<mutex> lock{cv_m};
        nextState = newState;
        if (newState == DeviceState::DeviceReady)
        {
            cv.notify_one();
            mServices.UnsubscribeFromDeviceStateChange("test");
        }
    });

    mDevice.WaitForState(fair::mq::State::InitializingDevice);
    mServices.ChangeDeviceState("foo", DeviceStateTransition::CompleteInit);
    mDevice.WaitForState(fair::mq::State::Initialized);
    mServices.ChangeDeviceState("foo", DeviceStateTransition::Bind);
    mDevice.WaitForState(fair::mq::State::Bound);
    mServices.ChangeDeviceState("foo", DeviceStateTransition::Connect);
    mDevice.WaitForState(fair::mq::State::DeviceReady);

    unique_lock<mutex> lock{cv_m};
    cv.wait(lock, [&]{ return nextState == DeviceState::DeviceReady; });

    ASSERT_EQ(mServices.GetCurrentDeviceState(), DeviceState::DeviceReady);

    mServices.ChangeDeviceState("foo", DeviceStateTransition::ResetDevice);
    mDevice.WaitForState(fair::mq::State::Idle);
    mServices.ChangeDeviceState("foo", DeviceStateTransition::End);
    mDevice.WaitForState(fair::mq::State::Exiting);
}

TEST_F(PluginServices, ControlStateConversions)
{
    const vector<pair<string, DeviceState>> states{
        {"UNDEFINED", DeviceState::Undefined},
        {"OK", DeviceState::Ok},
        {"ERROR", DeviceState::Error},
        {"IDLE", DeviceState::Idle},
        {"INITIALIZING DEVICE", DeviceState::InitializingDevice},
        {"INITIALIZED", DeviceState::Initialized},
        {"BINDING", DeviceState::Binding},
        {"BOUND", DeviceState::Bound},
        {"CONNECTING", DeviceState::Connecting},
        {"DEVICE READY", DeviceState::DeviceReady},
        {"INITIALIZING TASK", DeviceState::InitializingTask},
        {"READY", DeviceState::Ready},
        {"RUNNING", DeviceState::Running},
        {"RESETTING TASK", DeviceState::ResettingTask},
        {"RESETTING DEVICE", DeviceState::ResettingDevice},
        {"EXITING", DeviceState::Exiting},
    };
    for (auto const& [name, state] : states) {
        EXPECT_EQ(mServices.ToDeviceState(name), state) << name;
        EXPECT_EQ(mServices.ToStr(state), name) << name;
    }
    EXPECT_THROW(mServices.ToDeviceState("NO SUCH STATE"), std::out_of_range);
}

TEST_F(PluginServices, ControlStateTransitionConversions)
{
    const vector<pair<string, DeviceStateTransition>> transitions{
        {"AUTO", DeviceStateTransition::Auto},
        {"INIT DEVICE", DeviceStateTransition::InitDevice},
        {"COMPLETE INIT", DeviceStateTransition::CompleteInit},
        {"BIND", DeviceStateTransition::Bind},
        {"CONNECT", DeviceStateTransition::Connect},
        {"INIT TASK", DeviceStateTransition::InitTask},
        {"RUN", DeviceStateTransition::Run},
        {"STOP", DeviceStateTransition::Stop},
        {"RESET TASK", DeviceStateTransition::ResetTask},
        {"RESET DEVICE", DeviceStateTransition::ResetDevice},
        {"END", DeviceStateTransition::End},
        {"ERROR FOUND", DeviceStateTransition::ErrorFound},
    };
    for (auto const& [name, transition] : transitions) {
        EXPECT_EQ(mServices.ToDeviceStateTransition(name), transition) << name;
        EXPECT_EQ(mServices.ToStr(transition), name) << name;
    }
    EXPECT_THROW(mServices.ToDeviceStateTransition("NO SUCH TRANSITION"), std::out_of_range);
}

TEST_F(PluginServices, SubscriptionThreadSafety)
{
    // obviously not a perfect test, but I could segfault fmq reliably with it (without the fix)

    constexpr auto attempts = 1000;
    constexpr auto subscribers = 5;

    std::array<std::unique_ptr<std::thread>, subscribers> threads;
    auto id = 0;
    for (auto& thread : threads) {
        thread = std::make_unique<std::thread>([&, id](){
            auto const subscriber = fair::mq::tools::ToString("subscriber_", id);
            for (auto i = 0; i < attempts; ++i) {
                mServices.SubscribeToDeviceStateChange(subscriber, [](DeviceState){});
                mServices.UnsubscribeFromDeviceStateChange(subscriber);
            }
        });
        ++id;
    }

    for (auto& thread : threads) { thread->join(); }
}

} /* namespace */

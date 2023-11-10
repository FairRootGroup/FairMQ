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
#include <string>

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
    EXPECT_NO_THROW(mServices.ToDeviceState("OK"));
    EXPECT_NO_THROW(mServices.ToDeviceState("ERROR"));
    EXPECT_NO_THROW(mServices.ToDeviceState("IDLE"));
    EXPECT_NO_THROW(mServices.ToDeviceState("INITIALIZING DEVICE"));
    EXPECT_NO_THROW(mServices.ToDeviceState("BINDING"));
    EXPECT_NO_THROW(mServices.ToDeviceState("CONNECTING"));
    EXPECT_NO_THROW(mServices.ToDeviceState("DEVICE READY"));
    EXPECT_NO_THROW(mServices.ToDeviceState("INITIALIZING TASK"));
    EXPECT_NO_THROW(mServices.ToDeviceState("READY"));
    EXPECT_NO_THROW(mServices.ToDeviceState("RUNNING"));
    EXPECT_NO_THROW(mServices.ToDeviceState("RESETTING TASK"));
    EXPECT_NO_THROW(mServices.ToDeviceState("RESETTING DEVICE"));
    EXPECT_NO_THROW(mServices.ToDeviceState("EXITING"));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Ok));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Error));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Idle));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::InitializingDevice));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Binding));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Connecting));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::DeviceReady));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::InitializingTask));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Ready));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Running));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::ResettingTask));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::ResettingDevice));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Exiting));
}

TEST_F(PluginServices, ControlStateTransitionConversions)
{
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("INIT DEVICE"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("COMPLETE INIT"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("INIT TASK"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("RUN"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("STOP"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("RESET TASK"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("RESET DEVICE"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("END"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("ERROR FOUND"));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::InitDevice));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::CompleteInit));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::InitTask));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::Run));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::Stop));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::ResetTask));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::ResetDevice));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::End));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::ErrorFound));
}

TEST_F(PluginServices, SubscriptionThreadSafety)
{
    // obviously not a perfect test, but I could segfault fmq reliably with it (without the fix)

    constexpr auto attempts = 1000;
    constexpr auto subscribers = 5;

    std::array<std::unique_ptr<std::thread>, subscribers> threads;
    auto id = 0;
    for (auto& thread : threads) {
        thread = std::make_unique<std::thread>([&](){
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

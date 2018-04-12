/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Fixture.h"
#include <condition_variable>
#include <mutex>

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
    ASSERT_THROW( // no control for bar
        mServices.ReleaseDeviceControl("bar"),
        fair::mq::PluginServices::DeviceControlError
    );

    ASSERT_NO_THROW(mServices.ReleaseDeviceControl("foo"));
    ASSERT_FALSE(mServices.GetDeviceController());
    // take control implicitely
    ASSERT_NO_THROW(mServices.TakeDeviceControl("foo"));
    ASSERT_NO_THROW(mServices.ChangeDeviceState("foo", DeviceStateTransition::InitDevice));
    EXPECT_EQ(mServices.GetDeviceController(), string{"foo"});

    // park device
    mDevice->WaitForEndOfState(FairMQDevice::DEVICE_READY);
    mServices.ChangeDeviceState("foo", DeviceStateTransition::ResetDevice);
    mDevice->WaitForEndOfState(FairMQDevice::RESET_DEVICE);
    mServices.ChangeDeviceState("foo", DeviceStateTransition::End);
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

    unique_lock<mutex> lock{cv_m};
    cv.wait(lock);

    ASSERT_EQ(mServices.GetCurrentDeviceState(), DeviceState::DeviceReady);

    mServices.ChangeDeviceState("foo", DeviceStateTransition::ResetDevice);
    mDevice->WaitForEndOfState(FairMQDevice::RESET_DEVICE);
    mServices.ChangeDeviceState("foo", DeviceStateTransition::End);
}

TEST_F(PluginServices, ControlStateConversions)
{
    EXPECT_NO_THROW(mServices.ToDeviceState("OK"));
    EXPECT_NO_THROW(mServices.ToDeviceState("ERROR"));
    EXPECT_NO_THROW(mServices.ToDeviceState("IDLE"));
    EXPECT_NO_THROW(mServices.ToDeviceState("INITIALIZING DEVICE"));
    EXPECT_NO_THROW(mServices.ToDeviceState("DEVICE READY"));
    EXPECT_NO_THROW(mServices.ToDeviceState("INITIALIZING TASK"));
    EXPECT_NO_THROW(mServices.ToDeviceState("READY"));
    EXPECT_NO_THROW(mServices.ToDeviceState("RUNNING"));
    EXPECT_NO_THROW(mServices.ToDeviceState("PAUSED"));
    EXPECT_NO_THROW(mServices.ToDeviceState("RESETTING TASK"));
    EXPECT_NO_THROW(mServices.ToDeviceState("RESETTING DEVICE"));
    EXPECT_NO_THROW(mServices.ToDeviceState("EXITING"));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Ok));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Error));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Idle));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::InitializingDevice));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::DeviceReady));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::InitializingTask));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Ready));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Running));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Paused));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::ResettingTask));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::ResettingDevice));
    EXPECT_NO_THROW(mServices.ToStr(DeviceState::Exiting));
}

TEST_F(PluginServices, ControlStateTransitionConversions)
{
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("INIT DEVICE"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("INIT TASK"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("RUN"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("PAUSE"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("STOP"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("RESET TASK"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("RESET DEVICE"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("END"));
    EXPECT_NO_THROW(mServices.ToDeviceStateTransition("ERROR FOUND"));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::InitDevice));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::InitTask));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::Run));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::Pause));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::Stop));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::ResetTask));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::ResetDevice));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::End));
    EXPECT_NO_THROW(mServices.ToStr(DeviceStateTransition::ErrorFound));
}

} /* namespace */

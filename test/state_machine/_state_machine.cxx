/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <gtest/gtest.h>
#include <fairmq/StateMachine.h>
#include <fairlogger/Logger.h>
#include <string>
#include <thread>

namespace
{

using namespace std;
using namespace fair::mq;
using S = StateMachine::State;
using T = StateMachine::StateTransition;

TEST(StateMachine, RegularFSM)
{
    StateMachine fsm;

    ASSERT_FALSE(fsm.NextStatePending());

    ASSERT_NO_THROW(fsm.ChangeState(T::InitDevice));
    ASSERT_THROW(fsm.ChangeState(T::InitDevice), StateMachine::IllegalTransition);

    ASSERT_NO_THROW(fsm.ChangeState(T::Automatic));
    ASSERT_NO_THROW(fsm.ChangeState(T::InitTask));
    ASSERT_NO_THROW(fsm.ChangeState(T::Automatic));
    ASSERT_NO_THROW(fsm.ChangeState(T::Run));
    ASSERT_NO_THROW(fsm.ChangeState(T::Stop));
    ASSERT_NO_THROW(fsm.ChangeState(T::ResetTask));
    ASSERT_NO_THROW(fsm.ChangeState(T::Automatic));

    int cnt{0};
    fsm.SubscribeToStateQueued("test", [&](S /*newState*/, S /*lastState*/){
        ++cnt;
    });

    fsm.SubscribeToStateChange("test", [&](S newState, S lastState){
        if (newState == S::Idle && lastState == S::ResettingDevice) {
            ASSERT_NO_THROW(fsm.ChangeState(T::End));
        }
    });

    ASSERT_NO_THROW(fsm.ChangeState(T::ResetDevice));
    ASSERT_NO_THROW(fsm.ChangeState(T::Automatic));

    fsm.UnsubscribeFromStateQueued("test");

    ASSERT_TRUE(fsm.NextStatePending());

    fsm.Run();

    EXPECT_EQ(cnt, 2);
}

TEST(StateMachine, ErrorFSM)
{
    StateMachine fsm;

    ASSERT_NO_THROW(fsm.ChangeState(T::InitDevice));
    ASSERT_NO_THROW(fsm.ChangeState(T::Automatic));
    ASSERT_NO_THROW(fsm.ChangeState(T::ErrorFound));

    fsm.Run();
}

TEST(StateMachine, Reset)
{
    StateMachine fsm;

    ASSERT_NO_THROW(fsm.ChangeState(T::End));
    fsm.Run();

    fsm.Reset();

    ASSERT_NO_THROW(fsm.ChangeState(T::End));
    fsm.Run();
}

TEST(StateMachine, StateConversions)
{
    StateMachine fsm;
    EXPECT_NO_THROW(fsm.ToState("OK"));
    EXPECT_NO_THROW(fsm.ToState("ERROR"));
    EXPECT_NO_THROW(fsm.ToState("IDLE"));
    EXPECT_NO_THROW(fsm.ToState("INITIALIZING DEVICE"));
    EXPECT_NO_THROW(fsm.ToState("DEVICE READY"));
    EXPECT_NO_THROW(fsm.ToState("INITIALIZING TASK"));
    EXPECT_NO_THROW(fsm.ToState("READY"));
    EXPECT_NO_THROW(fsm.ToState("RUNNING"));
    EXPECT_NO_THROW(fsm.ToState("RESETTING TASK"));
    EXPECT_NO_THROW(fsm.ToState("RESETTING DEVICE"));
    EXPECT_NO_THROW(fsm.ToState("EXITING"));
    EXPECT_NO_THROW(fsm.ToStr(S::Ok));
    EXPECT_NO_THROW(fsm.ToStr(S::Error));
    EXPECT_NO_THROW(fsm.ToStr(S::Idle));
    EXPECT_NO_THROW(fsm.ToStr(S::InitializingDevice));
    EXPECT_NO_THROW(fsm.ToStr(S::DeviceReady));
    EXPECT_NO_THROW(fsm.ToStr(S::InitializingTask));
    EXPECT_NO_THROW(fsm.ToStr(S::Ready));
    EXPECT_NO_THROW(fsm.ToStr(S::Running));
    EXPECT_NO_THROW(fsm.ToStr(S::ResettingTask));
    EXPECT_NO_THROW(fsm.ToStr(S::ResettingDevice));
    EXPECT_NO_THROW(fsm.ToStr(S::Exiting));
}

TEST(StateMachine, StateTransitionConversions)
{
    StateMachine fsm;
    EXPECT_NO_THROW(fsm.ToStateTransition("INIT DEVICE"));
    EXPECT_NO_THROW(fsm.ToStateTransition("INIT TASK"));
    EXPECT_NO_THROW(fsm.ToStateTransition("RUN"));
    EXPECT_NO_THROW(fsm.ToStateTransition("STOP"));
    EXPECT_NO_THROW(fsm.ToStateTransition("RESET TASK"));
    EXPECT_NO_THROW(fsm.ToStateTransition("RESET DEVICE"));
    EXPECT_NO_THROW(fsm.ToStateTransition("END"));
    EXPECT_NO_THROW(fsm.ToStateTransition("ERROR FOUND"));
    EXPECT_NO_THROW(fsm.ToStateTransition("AUTOMATIC"));
    EXPECT_NO_THROW(fsm.ToStr(T::InitDevice));
    EXPECT_NO_THROW(fsm.ToStr(T::InitTask));
    EXPECT_NO_THROW(fsm.ToStr(T::Run));
    EXPECT_NO_THROW(fsm.ToStr(T::Stop));
    EXPECT_NO_THROW(fsm.ToStr(T::ResetTask));
    EXPECT_NO_THROW(fsm.ToStr(T::ResetDevice));
    EXPECT_NO_THROW(fsm.ToStr(T::End));
    EXPECT_NO_THROW(fsm.ToStr(T::ErrorFound));
    EXPECT_NO_THROW(fsm.ToStr(T::Automatic));
}

} // namespace

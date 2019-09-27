/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <gtest/gtest.h>
#include <fairmq/sdk/commands/Commands.h>

#include <string>

namespace
{

using namespace fair::mq;
using namespace fair::mq::sdk::cmd;

TEST(Format, Construction)
{
    Cmds checkStateCmds(make<CheckState>());
    Cmds changeStateCmds(make<ChangeState>(Transition::Stop));
    Cmds dumpConfigCmds(make<DumpConfig>());
    Cmds subscribeToHeartbeatsCmds(make<SubscribeToHeartbeats>());
    Cmds unsubscribeFromHeartbeatsCmds(make<UnsubscribeFromHeartbeats>());
    Cmds subscribeToStateChangeCmds(make<SubscribeToStateChange>());
    Cmds unsubscribeFromStateChangeCmds(make<UnsubscribeFromStateChange>());
    Cmds stateChangeExitingReceivedCmds(make<StateChangeExitingReceived>());
    Cmds currentStateCmds(make<CurrentState>("somedeviceid", State::Running));
    Cmds transitionStatusCmds(make<TransitionStatus>("somedeviceid", Result::Ok, Transition::Stop));
    Cmds configCmds(make<Config>("somedeviceid", "someconfig"));
    Cmds heartbeatSubscriptionCmds(make<HeartbeatSubscription>("somedeviceid", Result::Ok));
    Cmds heartbeatUnsubscriptionCmds(make<HeartbeatUnsubscription>("somedeviceid", Result::Ok));
    Cmds heartbeatCmds(make<Heartbeat>("somedeviceid"));
    Cmds stateChangeSubscriptionCmds(make<StateChangeSubscription>("somedeviceid", Result::Ok));
    Cmds stateChangeUnsubscriptionCmds(make<StateChangeUnsubscription>("somedeviceid", Result::Ok));
    Cmds stateChangeCmds(make<StateChange>("somedeviceid", 123456, State::Running, State::Ready));

    ASSERT_EQ(checkStateCmds.At(0).GetType(), Type::check_state);
    ASSERT_EQ(changeStateCmds.At(0).GetType(), Type::change_state);
    ASSERT_EQ(static_cast<ChangeState&>(changeStateCmds.At(0)).GetTransition(), Transition::Stop);
    ASSERT_EQ(dumpConfigCmds.At(0).GetType(), Type::dump_config);
    ASSERT_EQ(subscribeToHeartbeatsCmds.At(0).GetType(), Type::subscribe_to_heartbeats);
    ASSERT_EQ(unsubscribeFromHeartbeatsCmds.At(0).GetType(), Type::unsubscribe_from_heartbeats);
    ASSERT_EQ(subscribeToStateChangeCmds.At(0).GetType(), Type::subscribe_to_state_change);
    ASSERT_EQ(unsubscribeFromStateChangeCmds.At(0).GetType(), Type::unsubscribe_from_state_change);
    ASSERT_EQ(stateChangeExitingReceivedCmds.At(0).GetType(), Type::state_change_exiting_received);
    ASSERT_EQ(currentStateCmds.At(0).GetType(), Type::current_state);
    ASSERT_EQ(static_cast<CurrentState&>(currentStateCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<CurrentState&>(currentStateCmds.At(0)).GetCurrentState(), State::Running);
    ASSERT_EQ(transitionStatusCmds.At(0).GetType(), Type::transition_status);
    ASSERT_EQ(static_cast<TransitionStatus&>(transitionStatusCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<TransitionStatus&>(transitionStatusCmds.At(0)).GetResult(), Result::Ok);
    ASSERT_EQ(static_cast<TransitionStatus&>(transitionStatusCmds.At(0)).GetTransition(), Transition::Stop);
    ASSERT_EQ(configCmds.At(0).GetType(), Type::config);
    ASSERT_EQ(static_cast<Config&>(configCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<Config&>(configCmds.At(0)).GetConfig(), "someconfig");
    ASSERT_EQ(heartbeatSubscriptionCmds.At(0).GetType(), Type::heartbeat_subscription);
    ASSERT_EQ(static_cast<HeartbeatSubscription&>(heartbeatSubscriptionCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<HeartbeatSubscription&>(heartbeatSubscriptionCmds.At(0)).GetResult(), Result::Ok);
    ASSERT_EQ(heartbeatUnsubscriptionCmds.At(0).GetType(), Type::heartbeat_unsubscription);
    ASSERT_EQ(static_cast<HeartbeatUnsubscription&>(heartbeatUnsubscriptionCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<HeartbeatUnsubscription&>(heartbeatUnsubscriptionCmds.At(0)).GetResult(), Result::Ok);
    ASSERT_EQ(heartbeatCmds.At(0).GetType(), Type::heartbeat);
    ASSERT_EQ(static_cast<Heartbeat&>(heartbeatCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(stateChangeSubscriptionCmds.At(0).GetType(), Type::state_change_subscription);
    ASSERT_EQ(static_cast<StateChangeSubscription&>(stateChangeSubscriptionCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<StateChangeSubscription&>(stateChangeSubscriptionCmds.At(0)).GetResult(), Result::Ok);
    ASSERT_EQ(stateChangeUnsubscriptionCmds.At(0).GetType(), Type::state_change_unsubscription);
    ASSERT_EQ(static_cast<StateChangeUnsubscription&>(stateChangeUnsubscriptionCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<StateChangeUnsubscription&>(stateChangeUnsubscriptionCmds.At(0)).GetResult(), Result::Ok);
    ASSERT_EQ(stateChangeCmds.At(0).GetType(), Type::state_change);
    ASSERT_EQ(static_cast<StateChange&>(stateChangeCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<StateChange&>(stateChangeCmds.At(0)).GetTaskId(), 123456);
    ASSERT_EQ(static_cast<StateChange&>(stateChangeCmds.At(0)).GetLastState(), State::Running);
    ASSERT_EQ(static_cast<StateChange&>(stateChangeCmds.At(0)).GetCurrentState(), State::Ready);
}

TEST(Format, Serialization)
{
    Cmds outCmds;

    outCmds.Add<CheckState>();
    outCmds.Add<ChangeState>(Transition::Stop);
    outCmds.Add<DumpConfig>();
    outCmds.Add<SubscribeToHeartbeats>();
    outCmds.Add<UnsubscribeFromHeartbeats>();
    outCmds.Add<SubscribeToStateChange>();
    outCmds.Add<UnsubscribeFromStateChange>();
    outCmds.Add<StateChangeExitingReceived>();
    outCmds.Add<CurrentState>("somedeviceid", State::Running);
    outCmds.Add<TransitionStatus>("somedeviceid", Result::Ok, Transition::Stop);
    outCmds.Add<Config>("somedeviceid", "someconfig");
    outCmds.Add<HeartbeatSubscription>("somedeviceid", Result::Ok);
    outCmds.Add<HeartbeatUnsubscription>("somedeviceid", Result::Ok);
    outCmds.Add<Heartbeat>("somedeviceid");
    outCmds.Add<StateChangeSubscription>("somedeviceid", Result::Ok);
    outCmds.Add<StateChangeUnsubscription>("somedeviceid", Result::Ok);
    outCmds.Add<StateChange>("somedeviceid", 123456, State::Running, State::Ready);

    std::string buffer(outCmds.Serialize());

    Cmds inCmds;

    inCmds.Deserialize(buffer);

    ASSERT_EQ(inCmds.Size(), 17);

    int count = 0;

    for (const auto& cmd : inCmds) {
        switch (cmd->GetType()) {
            case Type::check_state:
                ++count;
            break;
            case Type::change_state:
                ++count;
                ASSERT_EQ(static_cast<ChangeState&>(*cmd).GetTransition(), Transition::Stop);
            break;
            case Type::dump_config:
                ++count;
            break;
            case Type::subscribe_to_heartbeats:
                ++count;
            break;
            case Type::unsubscribe_from_heartbeats:
                ++count;
            break;
            case Type::subscribe_to_state_change:
                ++count;
            break;
            case Type::unsubscribe_from_state_change:
                ++count;
            break;
            case Type::state_change_exiting_received:
                ++count;
            break;
            case Type::current_state:
                ++count;
                ASSERT_EQ(static_cast<CurrentState&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<CurrentState&>(*cmd).GetCurrentState(), State::Running);
            break;
            case Type::transition_status:
                ++count;
                ASSERT_EQ(static_cast<TransitionStatus&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<TransitionStatus&>(*cmd).GetResult(), Result::Ok);
                ASSERT_EQ(static_cast<TransitionStatus&>(*cmd).GetTransition(), Transition::Stop);
            break;
            case Type::config:
                ++count;
                ASSERT_EQ(static_cast<Config&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<Config&>(*cmd).GetConfig(), "someconfig");
            break;
            case Type::heartbeat_subscription:
                ++count;
                ASSERT_EQ(static_cast<HeartbeatSubscription&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<HeartbeatSubscription&>(*cmd).GetResult(), Result::Ok);
            break;
            case Type::heartbeat_unsubscription:
                ++count;
                ASSERT_EQ(static_cast<HeartbeatUnsubscription&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<HeartbeatUnsubscription&>(*cmd).GetResult(), Result::Ok);
            break;
            case Type::heartbeat:
                ++count;
                ASSERT_EQ(static_cast<Heartbeat&>(*cmd).GetDeviceId(), "somedeviceid");
            break;
            case Type::state_change_subscription:
                ++count;
                ASSERT_EQ(static_cast<StateChangeSubscription&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<StateChangeSubscription&>(*cmd).GetResult(), Result::Ok);
            break;
            case Type::state_change_unsubscription:
                ++count;
                ASSERT_EQ(static_cast<StateChangeUnsubscription&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<StateChangeUnsubscription&>(*cmd).GetResult(), Result::Ok);
            break;
            case Type::state_change:
                ++count;
                ASSERT_EQ(static_cast<StateChange&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<StateChange&>(*cmd).GetTaskId(), 123456);
                ASSERT_EQ(static_cast<StateChange&>(*cmd).GetLastState(), State::Running);
                ASSERT_EQ(static_cast<StateChange&>(*cmd).GetCurrentState(), State::Ready);
            break;
            default:
                ASSERT_TRUE(false);
            break;
        }
    }

    ASSERT_EQ(count, 17);
}

} // namespace

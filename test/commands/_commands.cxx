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
    auto const props(std::vector<std::pair<std::string, std::string>>({{"k1", "v1"}, {"k2", "v2"}}));

    Cmds checkStateCmds(make<CheckState>());
    Cmds changeStateCmds(make<ChangeState>(Transition::Stop));
    Cmds dumpConfigCmds(make<DumpConfig>());
    Cmds subscribeToStateChangeCmds(make<SubscribeToStateChange>(60000));
    Cmds unsubscribeFromStateChangeCmds(make<UnsubscribeFromStateChange>());
    Cmds stateChangeExitingReceivedCmds(make<StateChangeExitingReceived>());
    Cmds getPropertiesCmds(make<GetProperties>(66, "k[12]"));
    Cmds setPropertiesCmds(make<SetProperties>(42, props));
    Cmds subscriptionHeartbeatCmds(make<SubscriptionHeartbeat>(60000));
    Cmds currentStateCmds(make<CurrentState>("somedeviceid", State::Running));
    Cmds transitionStatusCmds(make<TransitionStatus>("somedeviceid", 123456, Result::Ok, Transition::Stop, State::Running));
    Cmds configCmds(make<Config>("somedeviceid", "someconfig"));
    Cmds stateChangeSubscriptionCmds(make<StateChangeSubscription>("somedeviceid", 123456, Result::Ok));
    Cmds stateChangeUnsubscriptionCmds(make<StateChangeUnsubscription>("somedeviceid", 123456, Result::Ok));
    Cmds stateChangeCmds(make<StateChange>("somedeviceid", 123456, State::Running, State::Ready));
    Cmds propertiesCmds(make<Properties>("somedeviceid", 66, Result::Ok, props));
    Cmds propertiesSetCmds(make<PropertiesSet>("somedeviceid", 42, Result::Ok));

    ASSERT_EQ(checkStateCmds.At(0).GetType(), Type::check_state);
    ASSERT_EQ(changeStateCmds.At(0).GetType(), Type::change_state);
    ASSERT_EQ(static_cast<ChangeState&>(changeStateCmds.At(0)).GetTransition(), Transition::Stop);
    ASSERT_EQ(dumpConfigCmds.At(0).GetType(), Type::dump_config);
    ASSERT_EQ(subscribeToStateChangeCmds.At(0).GetType(), Type::subscribe_to_state_change);
    ASSERT_EQ(static_cast<SubscribeToStateChange&>(subscribeToStateChangeCmds.At(0)).GetInterval(), 60000);
    ASSERT_EQ(unsubscribeFromStateChangeCmds.At(0).GetType(), Type::unsubscribe_from_state_change);
    ASSERT_EQ(stateChangeExitingReceivedCmds.At(0).GetType(), Type::state_change_exiting_received);
    ASSERT_EQ(getPropertiesCmds.At(0).GetType(), Type::get_properties);
    ASSERT_EQ(static_cast<GetProperties&>(getPropertiesCmds.At(0)).GetRequestId(), 66);
    ASSERT_EQ(static_cast<GetProperties&>(getPropertiesCmds.At(0)).GetQuery(), "k[12]");
    ASSERT_EQ(setPropertiesCmds.At(0).GetType(), Type::set_properties);
    ASSERT_EQ(static_cast<SetProperties&>(setPropertiesCmds.At(0)).GetRequestId(), 42);
    ASSERT_EQ(static_cast<SetProperties&>(setPropertiesCmds.At(0)).GetProps(), props);
    ASSERT_EQ(subscriptionHeartbeatCmds.At(0).GetType(), Type::subscription_heartbeat);
    ASSERT_EQ(static_cast<SubscriptionHeartbeat&>(subscriptionHeartbeatCmds.At(0)).GetInterval(), 60000);
    ASSERT_EQ(currentStateCmds.At(0).GetType(), Type::current_state);
    ASSERT_EQ(static_cast<CurrentState&>(currentStateCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<CurrentState&>(currentStateCmds.At(0)).GetCurrentState(), State::Running);
    ASSERT_EQ(transitionStatusCmds.At(0).GetType(), Type::transition_status);
    ASSERT_EQ(static_cast<TransitionStatus&>(transitionStatusCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<TransitionStatus&>(transitionStatusCmds.At(0)).GetTaskId(), 123456);
    ASSERT_EQ(static_cast<TransitionStatus&>(transitionStatusCmds.At(0)).GetResult(), Result::Ok);
    ASSERT_EQ(static_cast<TransitionStatus&>(transitionStatusCmds.At(0)).GetTransition(), Transition::Stop);
    ASSERT_EQ(static_cast<TransitionStatus&>(transitionStatusCmds.At(0)).GetCurrentState(), State::Running);
    ASSERT_EQ(configCmds.At(0).GetType(), Type::config);
    ASSERT_EQ(static_cast<Config&>(configCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<Config&>(configCmds.At(0)).GetConfig(), "someconfig");
    ASSERT_EQ(stateChangeSubscriptionCmds.At(0).GetType(), Type::state_change_subscription);
    ASSERT_EQ(static_cast<StateChangeSubscription&>(stateChangeSubscriptionCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<StateChangeSubscription&>(stateChangeSubscriptionCmds.At(0)).GetTaskId(), 123456);
    ASSERT_EQ(static_cast<StateChangeSubscription&>(stateChangeSubscriptionCmds.At(0)).GetResult(), Result::Ok);
    ASSERT_EQ(stateChangeUnsubscriptionCmds.At(0).GetType(), Type::state_change_unsubscription);
    ASSERT_EQ(static_cast<StateChangeUnsubscription&>(stateChangeUnsubscriptionCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<StateChangeUnsubscription&>(stateChangeUnsubscriptionCmds.At(0)).GetTaskId(), 123456);
    ASSERT_EQ(static_cast<StateChangeUnsubscription&>(stateChangeUnsubscriptionCmds.At(0)).GetResult(), Result::Ok);
    ASSERT_EQ(stateChangeCmds.At(0).GetType(), Type::state_change);
    ASSERT_EQ(static_cast<StateChange&>(stateChangeCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<StateChange&>(stateChangeCmds.At(0)).GetTaskId(), 123456);
    ASSERT_EQ(static_cast<StateChange&>(stateChangeCmds.At(0)).GetLastState(), State::Running);
    ASSERT_EQ(static_cast<StateChange&>(stateChangeCmds.At(0)).GetCurrentState(), State::Ready);
    ASSERT_EQ(propertiesCmds.At(0).GetType(), Type::properties);
    ASSERT_EQ(static_cast<Properties&>(propertiesCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<Properties&>(propertiesCmds.At(0)).GetRequestId(), 66);
    ASSERT_EQ(static_cast<Properties&>(propertiesCmds.At(0)).GetResult(), Result::Ok);
    ASSERT_EQ(static_cast<Properties&>(propertiesCmds.At(0)).GetProps(), props);
    ASSERT_EQ(propertiesSetCmds.At(0).GetType(), Type::properties_set);
    ASSERT_EQ(static_cast<PropertiesSet&>(propertiesSetCmds.At(0)).GetDeviceId(), "somedeviceid");
    ASSERT_EQ(static_cast<PropertiesSet&>(propertiesSetCmds.At(0)).GetRequestId(), 42);
    ASSERT_EQ(static_cast<PropertiesSet&>(propertiesSetCmds.At(0)).GetResult(), Result::Ok);
}

void fillCommands(Cmds& cmds)
{
    auto const props(std::vector<std::pair<std::string, std::string>>({{"k1", "v1"}, {"k2", "v2"}}));

    cmds.Add<CheckState>();
    cmds.Add<ChangeState>(Transition::Stop);
    cmds.Add<DumpConfig>();
    cmds.Add<SubscribeToStateChange>(60000);
    cmds.Add<UnsubscribeFromStateChange>();
    cmds.Add<StateChangeExitingReceived>();
    cmds.Add<GetProperties>(66, "k[12]");
    cmds.Add<SetProperties>(42, props);
    cmds.Add<SubscriptionHeartbeat>(60000);
    cmds.Add<CurrentState>("somedeviceid", State::Running);
    cmds.Add<TransitionStatus>("somedeviceid", 123456, Result::Ok, Transition::Stop, State::Running);
    cmds.Add<Config>("somedeviceid", "someconfig");
    cmds.Add<StateChangeSubscription>("somedeviceid", 123456, Result::Ok);
    cmds.Add<StateChangeUnsubscription>("somedeviceid", 123456, Result::Ok);
    cmds.Add<StateChange>("somedeviceid", 123456, State::Running, State::Ready);
    cmds.Add<Properties>("somedeviceid", 66, Result::Ok, props);
    cmds.Add<PropertiesSet>("somedeviceid", 42, Result::Ok);
}

void checkCommands(Cmds& cmds)
{
    ASSERT_EQ(cmds.Size(), 17);

    int count = 0;
    auto const props(std::vector<std::pair<std::string, std::string>>({{"k1", "v1"}, {"k2", "v2"}}));

    for (const auto& cmd : cmds) {
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
            case Type::subscribe_to_state_change:
                ++count;
                ASSERT_EQ(static_cast<SubscribeToStateChange&>(*cmd).GetInterval(), 60000);
            break;
            case Type::unsubscribe_from_state_change:
                ++count;
            break;
            case Type::state_change_exiting_received:
                ++count;
            break;
            case Type::get_properties:
                ++count;
                ASSERT_EQ(static_cast<GetProperties&>(*cmd).GetRequestId(), 66);
                ASSERT_EQ(static_cast<GetProperties&>(*cmd).GetQuery(), "k[12]");
            break;
            case Type::set_properties:
                ++count;
                ASSERT_EQ(static_cast<SetProperties&>(*cmd).GetRequestId(), 42);
                ASSERT_EQ(static_cast<SetProperties&>(*cmd).GetProps(), props);
            break;
            case Type::subscription_heartbeat:
                ++count;
                ASSERT_EQ(static_cast<SubscriptionHeartbeat&>(*cmd).GetInterval(), 60000);
            break;
            case Type::current_state:
                ++count;
                ASSERT_EQ(static_cast<CurrentState&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<CurrentState&>(*cmd).GetCurrentState(), State::Running);
            break;
            case Type::transition_status:
                ++count;
                ASSERT_EQ(static_cast<TransitionStatus&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<TransitionStatus&>(*cmd).GetTaskId(), 123456);
                ASSERT_EQ(static_cast<TransitionStatus&>(*cmd).GetResult(), Result::Ok);
                ASSERT_EQ(static_cast<TransitionStatus&>(*cmd).GetTransition(), Transition::Stop);
                ASSERT_EQ(static_cast<TransitionStatus&>(*cmd).GetCurrentState(), State::Running);
            break;
            case Type::config:
                ++count;
                ASSERT_EQ(static_cast<Config&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<Config&>(*cmd).GetConfig(), "someconfig");
            break;
            case Type::state_change_subscription:
                ++count;
                ASSERT_EQ(static_cast<StateChangeSubscription&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<StateChangeSubscription&>(*cmd).GetTaskId(), 123456);
                ASSERT_EQ(static_cast<StateChangeSubscription&>(*cmd).GetResult(), Result::Ok);
            break;
            case Type::state_change_unsubscription:
                ++count;
                ASSERT_EQ(static_cast<StateChangeUnsubscription&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<StateChangeUnsubscription&>(*cmd).GetTaskId(), 123456);
                ASSERT_EQ(static_cast<StateChangeUnsubscription&>(*cmd).GetResult(), Result::Ok);
            break;
            case Type::state_change:
                ++count;
                ASSERT_EQ(static_cast<StateChange&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<StateChange&>(*cmd).GetTaskId(), 123456);
                ASSERT_EQ(static_cast<StateChange&>(*cmd).GetLastState(), State::Running);
                ASSERT_EQ(static_cast<StateChange&>(*cmd).GetCurrentState(), State::Ready);
            break;
            case Type::properties:
                ++count;
                ASSERT_EQ(static_cast<Properties&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<Properties&>(*cmd).GetRequestId(), 66);
                ASSERT_EQ(static_cast<Properties&>(*cmd).GetResult(), Result::Ok);
                ASSERT_EQ(static_cast<Properties&>(*cmd).GetProps(), props);
            break;
            case Type::properties_set:
                ++count;
                ASSERT_EQ(static_cast<PropertiesSet&>(*cmd).GetDeviceId(), "somedeviceid");
                ASSERT_EQ(static_cast<PropertiesSet&>(*cmd).GetRequestId(), 42);
                ASSERT_EQ(static_cast<PropertiesSet&>(*cmd).GetResult(), Result::Ok);
            break;
            default:
                ASSERT_TRUE(false);
            break;
        }
    }

    ASSERT_EQ(count, 17);
}

TEST(Format, SerializationBinary)
{
    Cmds outCmds;
    fillCommands(outCmds);
    std::string buffer(outCmds.Serialize());

    Cmds inCmds;
    inCmds.Deserialize(buffer);
    checkCommands(inCmds);
}

TEST(Format, SerializationJSON)
{
    Cmds outCmds;
    fillCommands(outCmds);
    std::string buffer(outCmds.Serialize(Format::JSON));

    Cmds inCmds;
    inCmds.Deserialize(buffer, Format::JSON);
    checkCommands(inCmds);
}

} // namespace

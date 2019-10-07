/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Commands.h"

#include <fairmq/sdk/commands/CommandsFormatDef.h>
#include <fairmq/sdk/commands/CommandsFormat.h>

#include <flatbuffers/idl.h>

#include <array>

using namespace std;

namespace fair {
namespace mq {
namespace sdk {
namespace cmd {

array<Result, 2> fbResultToResult =
{
    {
        Result::Ok,
        Result::Failure
    }
};

array<FBResult, 2> resultToFBResult =
{
    {
        FBResult::FBResult_Ok,
        FBResult::FBResult_Failure
    }
};

array<string, 2> resultNames =
{
    {
        "Ok",
        "Failure"
    }
};

array<string, 17> typeNames =
{
    {
        "CheckState",
        "ChangeState",
        "DumpConfig",
        "SubscribeToHeartbeats",
        "UnsubscribeFromHeartbeats",
        "SubscribeToStateChange",
        "UnsubscribeFromStateChange",
        "StateChangeExitingReceived",

        "CurrentState",
        "TransitionStatus",
        "Config",
        "HeartbeatSubscription",
        "HeartbeatUnsubscription",
        "Heartbeat",
        "StateChangeSubscription",
        "StateChangeUnsubscription",
        "StateChange"
    }
};

array<fair::mq::State, 15> fbStateToMQState =
{
    {
        fair::mq::State::Ok,
        fair::mq::State::Error,
        fair::mq::State::Idle,
        fair::mq::State::InitializingDevice,
        fair::mq::State::Initialized,
        fair::mq::State::Binding,
        fair::mq::State::Bound,
        fair::mq::State::Connecting,
        fair::mq::State::DeviceReady,
        fair::mq::State::InitializingTask,
        fair::mq::State::Ready,
        fair::mq::State::Running,
        fair::mq::State::ResettingTask,
        fair::mq::State::ResettingDevice,
        fair::mq::State::Exiting
    }
};

array<sdk::cmd::FBState, 15> mqStateToFBState =
{
    {
        sdk::cmd::FBState_Ok,
        sdk::cmd::FBState_Error,
        sdk::cmd::FBState_Idle,
        sdk::cmd::FBState_InitializingDevice,
        sdk::cmd::FBState_Initialized,
        sdk::cmd::FBState_Binding,
        sdk::cmd::FBState_Bound,
        sdk::cmd::FBState_Connecting,
        sdk::cmd::FBState_DeviceReady,
        sdk::cmd::FBState_InitializingTask,
        sdk::cmd::FBState_Ready,
        sdk::cmd::FBState_Running,
        sdk::cmd::FBState_ResettingTask,
        sdk::cmd::FBState_ResettingDevice,
        sdk::cmd::FBState_Exiting
    }
};

array<fair::mq::Transition, 12> fbTransitionToMQTransition =
{
    {
        fair::mq::Transition::Auto,
        fair::mq::Transition::InitDevice,
        fair::mq::Transition::CompleteInit,
        fair::mq::Transition::Bind,
        fair::mq::Transition::Connect,
        fair::mq::Transition::InitTask,
        fair::mq::Transition::Run,
        fair::mq::Transition::Stop,
        fair::mq::Transition::ResetTask,
        fair::mq::Transition::ResetDevice,
        fair::mq::Transition::End,
        fair::mq::Transition::ErrorFound
    }
};

array<sdk::cmd::FBTransition, 12> mqTransitionToFBTransition =
{
    {
        sdk::cmd::FBTransition_Auto,
        sdk::cmd::FBTransition_InitDevice,
        sdk::cmd::FBTransition_CompleteInit,
        sdk::cmd::FBTransition_Bind,
        sdk::cmd::FBTransition_Connect,
        sdk::cmd::FBTransition_InitTask,
        sdk::cmd::FBTransition_Run,
        sdk::cmd::FBTransition_Stop,
        sdk::cmd::FBTransition_ResetTask,
        sdk::cmd::FBTransition_ResetDevice,
        sdk::cmd::FBTransition_End,
        sdk::cmd::FBTransition_ErrorFound
    }
};

array<FBCmd, 17> typeToFBCmd =
{
    {
        FBCmd::FBCmd_check_state,
        FBCmd::FBCmd_change_state,
        FBCmd::FBCmd_dump_config,
        FBCmd::FBCmd_subscribe_to_heartbeats,
        FBCmd::FBCmd_unsubscribe_from_heartbeats,
        FBCmd::FBCmd_subscribe_to_state_change,
        FBCmd::FBCmd_unsubscribe_from_state_change,
        FBCmd::FBCmd_state_change_exiting_received,
        FBCmd::FBCmd_current_state,
        FBCmd::FBCmd_transition_status,
        FBCmd::FBCmd_config,
        FBCmd::FBCmd_heartbeat_subscription,
        FBCmd::FBCmd_heartbeat_unsubscription,
        FBCmd::FBCmd_heartbeat,
        FBCmd::FBCmd_state_change_subscription,
        FBCmd::FBCmd_state_change_unsubscription,
        FBCmd::FBCmd_state_change
    }
};

array<Type, 17> fbCmdToType =
{
    {
        Type::check_state,
        Type::change_state,
        Type::dump_config,
        Type::subscribe_to_heartbeats,
        Type::unsubscribe_from_heartbeats,
        Type::subscribe_to_state_change,
        Type::unsubscribe_from_state_change,
        Type::state_change_exiting_received,
        Type::current_state,
        Type::transition_status,
        Type::config,
        Type::heartbeat_subscription,
        Type::heartbeat_unsubscription,
        Type::heartbeat,
        Type::state_change_subscription,
        Type::state_change_unsubscription,
        Type::state_change
    }
};

fair::mq::State GetMQState(const FBState state) { return fbStateToMQState.at(state); }
FBState GetFBState(const fair::mq::State state) { return mqStateToFBState.at(static_cast<int>(state)); }
fair::mq::Transition GetMQTransition(const FBTransition transition) { return fbTransitionToMQTransition.at(transition); }
FBTransition GetFBTransition(const fair::mq::Transition transition) { return mqTransitionToFBTransition.at(static_cast<int>(transition)); }

Result GetResult(const FBResult result) { return fbResultToResult.at(result); }
FBResult GetFBResult(const Result result) { return resultToFBResult.at(static_cast<int>(result)); }
string GetResultName(const Result result) { return resultNames.at(static_cast<int>(result)); }
string GetTypeName(const Type type) { return typeNames.at(static_cast<int>(type)); }

FBCmd GetFBCmd(const Type type) { return typeToFBCmd.at(static_cast<int>(type)); }

string Cmds::Serialize(const Format type) const
{
    flatbuffers::FlatBufferBuilder fbb;
    vector<flatbuffers::Offset<FBCommand>> commandOffsets;

    for (auto& cmd : fCmds) {
        flatbuffers::Offset<FBCommand> cmdOffset;
        unique_ptr<FBCommandBuilder> cmdBuilder; // delay the creation of the builder, because child strings need to be constructed first (which are conditional)

        switch (cmd->GetType()) {
            case Type::check_state: {
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
            }
            break;
            case Type::change_state: {
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_transition(GetFBTransition(static_cast<ChangeState&>(*cmd).GetTransition()));
            }
            break;
            case Type::dump_config: {
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
            }
            break;
            case Type::subscribe_to_heartbeats: {
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
            }
            break;
            case Type::unsubscribe_from_heartbeats: {
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
            }
            break;
            case Type::subscribe_to_state_change: {
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
            }
            break;
            case Type::unsubscribe_from_state_change: {
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
            }
            break;
            case Type::state_change_exiting_received: {
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
            }
            break;
            case Type::current_state: {
                auto deviceId = fbb.CreateString(static_cast<CurrentState&>(*cmd).GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_current_state(GetFBState(static_cast<CurrentState&>(*cmd).GetCurrentState()));
            }
            break;
            case Type::transition_status: {
                auto deviceId = fbb.CreateString(static_cast<TransitionStatus&>(*cmd).GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_result(GetFBResult(static_cast<TransitionStatus&>(*cmd).GetResult()));
                cmdBuilder->add_transition(GetFBTransition(static_cast<TransitionStatus&>(*cmd).GetTransition()));
            }
            break;
            case Type::config: {
                auto deviceId = fbb.CreateString(static_cast<Config&>(*cmd).GetDeviceId());
                auto config = fbb.CreateString(static_cast<Config&>(*cmd).GetConfig());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_config_string(config);
            }
            break;
            case Type::heartbeat_subscription: {
                auto deviceId = fbb.CreateString(static_cast<HeartbeatSubscription&>(*cmd).GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_result(GetFBResult(static_cast<HeartbeatSubscription&>(*cmd).GetResult()));
            }
            break;
            case Type::heartbeat_unsubscription: {
                auto deviceId = fbb.CreateString(static_cast<HeartbeatUnsubscription&>(*cmd).GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_result(GetFBResult(static_cast<HeartbeatUnsubscription&>(*cmd).GetResult()));
            }
            break;
            case Type::heartbeat: {
                auto deviceId = fbb.CreateString(static_cast<Heartbeat&>(*cmd).GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
            }
            break;
            case Type::state_change_subscription: {
                auto deviceId = fbb.CreateString(static_cast<StateChangeSubscription&>(*cmd).GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_result(GetFBResult(static_cast<StateChangeSubscription&>(*cmd).GetResult()));
            }
            break;
            case Type::state_change_unsubscription: {
                auto deviceId = fbb.CreateString(static_cast<StateChangeUnsubscription&>(*cmd).GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_result(GetFBResult(static_cast<StateChangeUnsubscription&>(*cmd).GetResult()));
            }
            break;
            case Type::state_change: {
                auto deviceId = fbb.CreateString(static_cast<StateChange&>(*cmd).GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_task_id(static_cast<StateChange&>(*cmd).GetTaskId());
                cmdBuilder->add_last_state(GetFBState(static_cast<StateChange&>(*cmd).GetLastState()));
                cmdBuilder->add_current_state(GetFBState(static_cast<StateChange&>(*cmd).GetCurrentState()));
            }
            break;
            default:
                throw CommandFormatError("unrecognized command type given to fair::mq::cmd::Cmds::Serialize()");
            break;
        }

        cmdBuilder->add_command_id(typeToFBCmd.at(static_cast<int>(cmd->GetType())));

        cmdOffset = cmdBuilder->Finish();
        commandOffsets.push_back(cmdOffset);
    }

    auto commands = fbb.CreateVector(commandOffsets);
    auto cmds = CreateFBCommands(fbb, commands);
    fbb.Finish(cmds);

    if (type == Format::Binary) {
        return string(reinterpret_cast<char*>(fbb.GetBufferPointer()), fbb.GetSize());
    } else { // Type == Format::JSON
        flatbuffers::Parser parser;
        if (!parser.Parse(commandsFormatDefFbs)) {
            throw CommandFormatError("Serialize couldn't parse commands format");
        }
        std::string json;
        if (!flatbuffers::GenerateText(parser, fbb.GetBufferPointer(), &json)) {
            throw CommandFormatError("Serialize couldn't serialize parsed data to JSON!");
        }
        return json;
    }
}

void Cmds::Deserialize(const string& str, const Format type)
{
    fCmds.clear();

    const flatbuffers::Vector<flatbuffers::Offset<FBCommand>>* cmds;

    if (type == Format::Binary) {
        cmds = cmd::GetFBCommands(const_cast<char*>(str.c_str()))->commands();
    } else { // Type == Format::JSON
        flatbuffers::Parser parser;
        if (!parser.Parse(commandsFormatDefFbs)) {
            throw CommandFormatError("Deserialize couldn't parse commands format");
        }
        if (!parser.Parse(str.c_str())) {
            throw CommandFormatError("Deserialize couldn't parse incoming JSON string");
        }
        cmds = cmd::GetFBCommands(parser.builder_.GetBufferPointer())->commands();
    }

    for (unsigned int i = 0; i < cmds->size(); ++i) {
        const fair::mq::sdk::cmd::FBCommand& cmdPtr = *(cmds->Get(i));
        switch (cmdPtr.command_id()) {
            case FBCmd_check_state:
                fCmds.emplace_back(make<CheckState>());
            break;
            case FBCmd_change_state:
                fCmds.emplace_back(make<ChangeState>(GetMQTransition(cmdPtr.transition())));
            break;
            case FBCmd_dump_config:
                fCmds.emplace_back(make<DumpConfig>());
            break;
            case FBCmd_subscribe_to_heartbeats:
                fCmds.emplace_back(make<SubscribeToHeartbeats>());
            break;
            case FBCmd_unsubscribe_from_heartbeats:
                fCmds.emplace_back(make<UnsubscribeFromHeartbeats>());
            break;
            case FBCmd_subscribe_to_state_change:
                fCmds.emplace_back(make<SubscribeToStateChange>());
            break;
            case FBCmd_unsubscribe_from_state_change:
                fCmds.emplace_back(make<UnsubscribeFromStateChange>());
            break;
            case FBCmd_state_change_exiting_received:
                fCmds.emplace_back(make<StateChangeExitingReceived>());
            break;
            case FBCmd_current_state:
                fCmds.emplace_back(make<CurrentState>(cmdPtr.device_id()->str(), GetMQState(cmdPtr.current_state())));
            break;
            case FBCmd_transition_status:
                fCmds.emplace_back(make<TransitionStatus>(cmdPtr.device_id()->str(), GetResult(cmdPtr.result()), GetMQTransition(cmdPtr.transition())));
            break;
            case FBCmd_config:
                fCmds.emplace_back(make<Config>(cmdPtr.device_id()->str(), cmdPtr.config_string()->str()));
            break;
            case FBCmd_heartbeat_subscription:
                fCmds.emplace_back(make<HeartbeatSubscription>(cmdPtr.device_id()->str(), GetResult(cmdPtr.result())));
            break;
            case FBCmd_heartbeat_unsubscription:
                fCmds.emplace_back(make<HeartbeatUnsubscription>(cmdPtr.device_id()->str(), GetResult(cmdPtr.result())));
            break;
            case FBCmd_heartbeat:
                fCmds.emplace_back(make<Heartbeat>(cmdPtr.device_id()->str()));
            break;
            case FBCmd_state_change_subscription:
                fCmds.emplace_back(make<StateChangeSubscription>(cmdPtr.device_id()->str(), GetResult(cmdPtr.result())));
            break;
            case FBCmd_state_change_unsubscription:
                fCmds.emplace_back(make<StateChangeUnsubscription>(cmdPtr.device_id()->str(), GetResult(cmdPtr.result())));
            break;
            case FBCmd_state_change:
                fCmds.emplace_back(make<StateChange>(cmdPtr.device_id()->str(), cmdPtr.task_id(), GetMQState(cmdPtr.last_state()), GetMQState(cmdPtr.current_state())));
            break;
            default:
                throw CommandFormatError("unrecognized command type given to fair::mq::cmd::Cmds::Deserialize()");
            break;
        }
    }
}

}   // namespace cmd
}   // namespace sdk
}   // namespace mq
}   // namespace fair

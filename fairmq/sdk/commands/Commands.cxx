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

array<string, 21> typeNames =
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
        "GetProperties",
        "SetProperties",

        "CurrentState",
        "TransitionStatus",
        "Config",
        "HeartbeatSubscription",
        "HeartbeatUnsubscription",
        "Heartbeat",
        "StateChangeSubscription",
        "StateChangeUnsubscription",
        "StateChange",
        "Properties",
        "PropertiesSet"
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

array<FBCmd, 21> typeToFBCmd =
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
        FBCmd::FBCmd_get_properties,
        FBCmd::FBCmd_set_properties,
        FBCmd::FBCmd_current_state,
        FBCmd::FBCmd_transition_status,
        FBCmd::FBCmd_config,
        FBCmd::FBCmd_heartbeat_subscription,
        FBCmd::FBCmd_heartbeat_unsubscription,
        FBCmd::FBCmd_heartbeat,
        FBCmd::FBCmd_state_change_subscription,
        FBCmd::FBCmd_state_change_unsubscription,
        FBCmd::FBCmd_state_change,
        FBCmd::FBCmd_properties,
        FBCmd::FBCmd_properties_set
    }
};

array<Type, 21> fbCmdToType =
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
        Type::get_properties,
        Type::set_properties,
        Type::current_state,
        Type::transition_status,
        Type::config,
        Type::heartbeat_subscription,
        Type::heartbeat_unsubscription,
        Type::heartbeat,
        Type::state_change_subscription,
        Type::state_change_unsubscription,
        Type::state_change,
        Type::properties,
        Type::properties_set
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
            case Type::get_properties: {
                auto _cmd = static_cast<GetProperties&>(*cmd);
                auto query = fbb.CreateString(_cmd.GetQuery());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_request_id(_cmd.GetRequestId());
                cmdBuilder->add_property_query(query);
            }
            break;
            case Type::set_properties: {
                auto _cmd = static_cast<SetProperties&>(*cmd);
                std::vector<flatbuffers::Offset<FBProperty>> propsVector;
                for (auto const& e : _cmd.GetProps()) {
                    auto const key(fbb.CreateString(e.first));
                    auto const val(fbb.CreateString(e.second));
                    propsVector.push_back(CreateFBProperty(fbb, key, val));
                }
                auto props = fbb.CreateVector(propsVector);
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_request_id(_cmd.GetRequestId());
                cmdBuilder->add_properties(props);
            }
            break;
            case Type::current_state: {
                auto _cmd = static_cast<CurrentState&>(*cmd);
                auto deviceId = fbb.CreateString(_cmd.GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_current_state(GetFBState(_cmd.GetCurrentState()));
            }
            break;
            case Type::transition_status: {
                auto _cmd = static_cast<TransitionStatus&>(*cmd);
                auto deviceId = fbb.CreateString(_cmd.GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_result(GetFBResult(_cmd.GetResult()));
                cmdBuilder->add_transition(GetFBTransition(_cmd.GetTransition()));
            }
            break;
            case Type::config: {
                auto _cmd = static_cast<Config&>(*cmd);
                auto deviceId = fbb.CreateString(_cmd.GetDeviceId());
                auto config = fbb.CreateString(_cmd.GetConfig());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_config_string(config);
            }
            break;
            case Type::heartbeat_subscription: {
                auto _cmd = static_cast<HeartbeatSubscription&>(*cmd);
                auto deviceId = fbb.CreateString(_cmd.GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_result(GetFBResult(_cmd.GetResult()));
            }
            break;
            case Type::heartbeat_unsubscription: {
                auto _cmd = static_cast<HeartbeatUnsubscription&>(*cmd);
                auto deviceId = fbb.CreateString(_cmd.GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_result(GetFBResult(_cmd.GetResult()));
            }
            break;
            case Type::heartbeat: {
                auto deviceId = fbb.CreateString(static_cast<Heartbeat&>(*cmd).GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
            }
            break;
            case Type::state_change_subscription: {
                auto _cmd = static_cast<StateChangeSubscription&>(*cmd);
                auto deviceId = fbb.CreateString(_cmd.GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_result(GetFBResult(_cmd.GetResult()));
            }
            break;
            case Type::state_change_unsubscription: {
                auto _cmd = static_cast<StateChangeUnsubscription&>(*cmd);
                auto deviceId = fbb.CreateString(_cmd.GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_result(GetFBResult(_cmd.GetResult()));
            }
            break;
            case Type::state_change: {
                auto _cmd = static_cast<StateChange&>(*cmd);
                auto deviceId = fbb.CreateString(_cmd.GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_task_id(_cmd.GetTaskId());
                cmdBuilder->add_last_state(GetFBState(_cmd.GetLastState()));
                cmdBuilder->add_current_state(GetFBState(_cmd.GetCurrentState()));
            }
            break;
            case Type::properties: {
                auto _cmd = static_cast<Properties&>(*cmd);
                auto deviceId = fbb.CreateString(_cmd.GetDeviceId());

                std::vector<flatbuffers::Offset<FBProperty>> propsVector;
                for (const auto& e : _cmd.GetProps()) {
                    auto key = fbb.CreateString(e.first);
                    auto val = fbb.CreateString(e.second);
                    auto prop = CreateFBProperty(fbb, key, val);
                    propsVector.push_back(prop);
                }
                auto props = fbb.CreateVector(propsVector);
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_request_id(_cmd.GetRequestId());
                cmdBuilder->add_result(GetFBResult(_cmd.GetResult()));
                cmdBuilder->add_properties(props);
            }
            break;
            case Type::properties_set: {
                auto _cmd = static_cast<PropertiesSet&>(*cmd);
                auto deviceId = fbb.CreateString(_cmd.GetDeviceId());
                cmdBuilder = tools::make_unique<FBCommandBuilder>(fbb);
                cmdBuilder->add_device_id(deviceId);
                cmdBuilder->add_request_id(_cmd.GetRequestId());
                cmdBuilder->add_result(GetFBResult(_cmd.GetResult()));
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
            case FBCmd_get_properties:
                fCmds.emplace_back(make<GetProperties>(cmdPtr.request_id(), cmdPtr.property_query()->str()));
            break;
            case FBCmd_set_properties: {
                std::vector<std::pair<std::string, std::string>> properties;
                auto props = cmdPtr.properties();
                for (unsigned int j = 0; j < props->size(); ++j) {
                    properties.emplace_back(props->Get(j)->key()->str(), props->Get(j)->value()->str());
                }
                fCmds.emplace_back(make<SetProperties>(cmdPtr.request_id(), properties));
            } break;
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
            case FBCmd_properties: {
                std::vector<std::pair<std::string, std::string>> properties;
                auto props = cmdPtr.properties();
                for (unsigned int j = 0; j < props->size(); ++j) {
                    properties.emplace_back(props->Get(j)->key()->str(), props->Get(j)->value()->str());
                }
                fCmds.emplace_back(make<Properties>(cmdPtr.device_id()->str(), cmdPtr.request_id(), GetResult(cmdPtr.result()), properties));
            } break;
            case FBCmd_properties_set:
                fCmds.emplace_back(make<PropertiesSet>(cmdPtr.device_id()->str(), cmdPtr.request_id(), GetResult(cmdPtr.result())));
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

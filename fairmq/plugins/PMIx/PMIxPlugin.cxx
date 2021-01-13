/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "PMIxPlugin.h"

#include <fairmq/sdk/commands/Commands.h>
#include <fairmq/tools/Strings.h>

#include <sstream>
#include <stdexcept>
#include <cstdint> // UINT32_MAX

using namespace std;
using namespace fair::mq::sdk::cmd;

namespace fair::mq::plugins
{

PMIxPlugin::PMIxPlugin(const string& name,
                       const Plugin::Version version,
                       const string& maintainer,
                       const string& homepage,
                       PluginServices* pluginServices)
    : Plugin(name, version, maintainer, homepage, pluginServices)
    , fProcess(Init())
    , fPid(getpid())
    , fPMIxClient(tools::ToString("PMIx client(pid=", fPid, ") "))
    , fDeviceId(string(fProcess.nspace) + "_" + to_string(fProcess.rank))
    , fCommands(fProcess)
    , fLastExternalController(UINT32_MAX)
    , fExitingAckedByLastExternalController(false)
    , fCurrentState(DeviceState::Idle)
    , fLastState(DeviceState::Idle)
{
    TakeDeviceControl();
    LOG(debug) << PMIxClient() << "pmix::init() OK: " << fProcess << ", version=" << pmix::get_version();
    SetProperty<string>("id", fDeviceId);

    Fence("pmix::init");
    SubscribeForCommands();
    Fence("subscribed");

    // fCommands.Send("test1");
    // fCommands.Send("test2", 0);
    // fCommands.Send("test3", 0);

    // LOG(info) << "PMIX_EXTERNAL_ERR_BASE: " << PMIX_EXTERNAL_ERR_BASE;

    // job level infos
    // LOG(info) << "PMIX_SESSION_ID: "         << pmix::getInfo(PMIX_SESSION_ID, fProcess);
    // LOG(info) << "PMIX_UNIV_SIZE: "          << pmix::getInfo(PMIX_UNIV_SIZE, fProcess);
    // LOG(info) << "PMIX_JOB_SIZE: "           << pmix::getInfo(PMIX_JOB_SIZE, fProcess);
    // LOG(info) << "PMIX_JOB_NUM_APPS: "       << pmix::getInfo(PMIX_JOB_NUM_APPS, fProcess);
    // LOG(info) << "PMIX_APP_SIZE: "           << pmix::getInfo(PMIX_APP_SIZE, fProcess);
    // LOG(info) << "PMIX_MAX_PROCS: "          << pmix::getInfo(PMIX_MAX_PROCS, fProcess);
    // LOG(info) << "PMIX_NUM_NODES: "          << pmix::getInfo(PMIX_NUM_NODES, fProcess);
    // LOG(info) << "PMIX_CLUSTER_ID: "         << pmix::getInfo(PMIX_CLUSTER_ID, fProcess);
    // LOG(info) << "PMIX_NSPACE: "             << pmix::getInfo(PMIX_NSPACE, fProcess);
    // LOG(info) << "PMIX_JOBID: "              << pmix::getInfo(PMIX_JOBID, fProcess);
    // LOG(info) << "PMIX_NODE_LIST: "          << pmix::getInfo(PMIX_NODE_LIST, fProcess);
    // LOG(info) << "PMIX_ALLOCATED_NODELIST: " << pmix::getInfo(PMIX_ALLOCATED_NODELIST, fProcess);
    // LOG(info) << "PMIX_NPROC_OFFSET: "       << pmix::getInfo(PMIX_NPROC_OFFSET, fProcess);
    // LOG(info) << "PMIX_LOCALLDR: "           << pmix::getInfo(PMIX_LOCALLDR, fProcess);
    // LOG(info) << "PMIX_APPLDR: "             << pmix::getInfo(PMIX_APPLDR, fProcess);

    // // per-node information
    // LOG(info) << "PMIX_NODE_SIZE: "          << pmix::getInfo(PMIX_NODE_SIZE, fProcess);
    // LOG(info) << "PMIX_LOCAL_SIZE: "         << pmix::getInfo(PMIX_LOCAL_SIZE, fProcess);
    // LOG(info) << "PMIX_AVAIL_PHYS_MEMORY: "  << pmix::getInfo(PMIX_AVAIL_PHYS_MEMORY, fProcess);

    // // per-process information
    // LOG(info) << "PMIX_PROCID: "             << pmix::getInfo(PMIX_PROCID, fProcess);
    // LOG(info) << "PMIX_APPNUM: "             << pmix::getInfo(PMIX_APPNUM, fProcess);
    // LOG(info) << "PMIX_LOCAL_RANK: "         << pmix::getInfo(PMIX_LOCAL_RANK, fProcess);
    // LOG(info) << "PMIX_NODE_RANK: "          << pmix::getInfo(PMIX_NODE_RANK, fProcess);
    // LOG(info) << "PMIX_RANK: "               << pmix::getInfo(PMIX_RANK, fProcess);
    // LOG(info) << "PMIX_GLOBAL_RANK: "        << pmix::getInfo(PMIX_GLOBAL_RANK, fProcess);
    // LOG(info) << "PMIX_APP_RANK: "           << pmix::getInfo(PMIX_APP_RANK, fProcess);

    SubscribeToDeviceStateChange([this](DeviceState newState) {
        switch (newState) {
            case DeviceState::Bound:
                Publish();
                break;
            case DeviceState::Connecting:
                Lookup();
                break;
            case DeviceState::Exiting:
                ReleaseDeviceControl();
                UnsubscribeFromDeviceStateChange();
                break;
            default:
                break;
        }

        lock_guard<mutex> lock{fStateChangeSubscriberMutex};
        fLastState = fCurrentState;
        fCurrentState = newState;
        for (auto subscriberId : fStateChangeSubscribers) {
            LOG(debug) << "Publishing state-change: " << fLastState << "->" << newState << " to " << subscriberId;
            Cmds cmds(make<StateChange>(fDeviceId, 0, fLastState, fCurrentState));
            fCommands.Send(cmds.Serialize(Format::JSON), static_cast<pmix::rank>(subscriberId));
        }
    });
}

PMIxPlugin::~PMIxPlugin()
{
    LOG(debug) << "Destroying PMIxPlugin";
    ReleaseDeviceControl();
    fCommands.Unsubscribe();
    while (pmix::initialized()) {
        try {
            pmix::finalize();
            LOG(debug) << PMIxClient() << "pmix::finalize() OK";
        } catch (const pmix::runtime_error& e) {
            LOG(debug) << PMIxClient() << "pmix::finalize() failed: " << e.what();
        }
    }
}

auto PMIxPlugin::SubscribeForCommands() -> void
{
    fCommands.Subscribe([this](const string& cmdStr, const pmix::proc& sender) {
        // LOG(info) << "PMIx Plugin received message: '" << cmdStr << "', from " << sender;

        Cmds inCmds;
        inCmds.Deserialize(cmdStr, Format::JSON);

        for (const auto& cmd : inCmds) {
            LOG(info) << "Received command type: '" << cmd->GetType() << "' from " << sender;
            switch (cmd->GetType()) {
                case Type::check_state:
                    fCommands.Send(Cmds(make<CurrentState>(fDeviceId, GetCurrentDeviceState()))
                                       .Serialize(Format::JSON),
                                   {sender});
                    break;
                case Type::change_state: {
                    Transition transition = static_cast<ChangeState&>(*cmd).GetTransition();
                    if (ChangeDeviceState(transition)) {
                        fCommands.Send(
                            Cmds(make<TransitionStatus>(fDeviceId, 0, Result::Ok, transition, GetCurrentDeviceState()))
                                .Serialize(Format::JSON),
                            {sender});
                    } else {
                        fCommands.Send(
                            Cmds(make<TransitionStatus>(fDeviceId, 0, Result::Failure, transition, GetCurrentDeviceState()))
                                .Serialize(Format::JSON),
                            {sender});
                    }
                    {
                        lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                        fLastExternalController = sender.rank;
                    }
                }
                break;
                case Type::subscribe_to_state_change: {
                    {
                        lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                        fStateChangeSubscribers.insert(sender.rank);
                    }

                    LOG(debug) << "Publishing state-change: " << fLastState << "->" << fCurrentState
                               << " to " << sender;
                    Cmds outCmds(make<StateChangeSubscription>(fDeviceId, fProcess.rank, Result::Ok),
                                 make<StateChange>(fDeviceId, 0, fLastState, fCurrentState));
                    fCommands.Send(outCmds.Serialize(Format::JSON), {sender});
                }
                break;
                case Type::unsubscribe_from_state_change: {
                    {
                        lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                        fStateChangeSubscribers.erase(sender.rank);
                    }
                    fCommands.Send(Cmds(make<StateChangeUnsubscription>(fDeviceId, fProcess.rank, Result::Ok))
                                       .Serialize(Format::JSON),
                                   {sender});
                }
                break;
                case Type::state_change_exiting_received: {
                    {
                        lock_guard<mutex> lock{fStateChangeSubscriberMutex};
                        if (fLastExternalController == sender.rank) {
                            fExitingAckedByLastExternalController = true;
                        }
                    }
                    fExitingAcked.notify_one();
                }
                break;
                case Type::dump_config: {
                    stringstream ss;
                    for (const auto& k: GetPropertyKeys()) {
                        ss << fDeviceId << ": " << k << " -> " << GetPropertyAsString(k) << "\n";
                    }
                    fCommands.Send(Cmds(make<Config>(fDeviceId, ss.str())).Serialize(Format::JSON),
                                   {sender});
                }
                break;
                default:
                    LOG(warn) << "Unexpected/unknown command received: " << cmdStr;
                    LOG(warn) << "Origin: " << sender;
                break;
            }
        }
    });
}

auto PMIxPlugin::Init() -> pmix::proc
{
    if (!pmix::initialized()) {
        return pmix::init();
    } else {
        throw runtime_error("trying to initialize PMIx while it is already initialized");
    }
}

auto PMIxPlugin::Publish() -> void
{
    auto channels(GetChannelInfo());
    vector<pmix::info> info;

    for (const auto& c : channels) {
        string methodKey("chans." + c.first + "." + to_string(c.second - 1) + ".method");
        if (GetProperty<string>(methodKey) == "bind") {
            for (int i = 0; i < c.second; ++i) {
                string addressKey("chans." + c.first + "." + to_string(i) + ".address");
                info.emplace_back(addressKey, GetProperty<string>(addressKey));
                LOG(debug) << PMIxClient() << info.back();
            }
        }
    }

    if (info.size() > 0) {
        pmix::publish(info);
        LOG(debug) << PMIxClient() << "pmix::publish() OK: published " << info.size()
                   << " binding channels.";
    }
}

auto PMIxPlugin::Fence() -> void
{
    pmix::proc all(fProcess);
    all.rank = pmix::rank::wildcard;

    pmix::fence({all});
}

auto PMIxPlugin::Fence(const std::string& label) -> void
{
    Fence(label);
    LOG(debug) << PMIxClient() << "pmix::fence() [" << label << "] OK";
}

auto PMIxPlugin::Lookup() -> void
{
    auto channels(GetChannelInfo());
    for (const auto& c : channels) {
        string methodKey("chans." + c.first + "." + to_string(c.second - 1) + ".method");
        if (GetProperty<string>(methodKey) == "connect") {
            for (int i = 0; i < c.second; ++i) {
                vector<pmix::pdata> pdata;
                string addressKey("chans." + c.first + "." + to_string(i) + ".address");
                pdata.emplace_back();
                pdata.back().set_key(addressKey);
                vector<pmix::info> info;
                info.emplace_back(PMIX_WAIT, static_cast<int>(pdata.size()));

                if (pdata.size() > 0) {
                    pmix::lookup(pdata, info);
                    LOG(debug) << PMIxClient() << "pmix::lookup() OK";
                }

                for (const auto& p : pdata) {
                    if (p.value.type == PMIX_UNDEF) {
                        LOG(debug) << PMIxClient() << "pmix::lookup() not found: key=" << p.key;
                    } else if (p.value.type == PMIX_STRING) {
                        LOG(debug) << PMIxClient() << "pmix::lookup() found:"
                                   << " key=" << p.key << ",value=" << p.value.data.string;
                        SetProperty<string>(p.key, p.value.data.string);
                    } else {
                        LOG(debug) << PMIxClient() << "pmix::lookup() wrong type returned: "
                                   << "key=" << p.key << ",type=" << p.value.type;
                    }
                }
            }
        }
    }
}

auto PMIxPlugin::WaitForExitingAck() -> void
{
    unique_lock<mutex> lock(fStateChangeSubscriberMutex);
    fExitingAcked.wait_for(lock, chrono::milliseconds(1000), [this]() {
        return fExitingAckedByLastExternalController;
    });
}

} // namespace fair::mq::plugins

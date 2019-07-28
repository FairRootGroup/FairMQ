/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DDSSession.h"

#include <fairmq/sdk/DDSEnvironment.h>
#include <fairmq/sdk/DDSTopology.h>
#include <fairmq/Tools.h>

#include <fairlogger/Logger.h>

#include <DDS/Tools.h>

#include <boost/uuid/uuid_io.hpp>

#include <cassert>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace fair {
namespace mq {
namespace sdk {

auto operator<<(std::ostream& os, DDSRMSPlugin plugin) -> std::ostream&
{
    switch (plugin) {
        case DDSRMSPlugin::ssh:
            return os << "ssh";
        case DDSRMSPlugin::localhost:
            return os << "localhost";
        default:
            __builtin_unreachable();
    }
}

auto operator>>(std::istream& is, DDSRMSPlugin& plugin) -> std::istream&
{
    std::string value;
    if (is >> value) {
        if (value == "ssh") {
            plugin = DDSRMSPlugin::ssh;
        } else if (value == "localhost") {
            plugin = DDSRMSPlugin::localhost;
        } else {
            throw std::runtime_error("Unknown or unsupported DDSRMSPlugin");
        }
    }
    return is;
}

struct DDSSession::Impl
{
    explicit Impl(DDSEnvironment env)
        : fEnv(std::move(env))
        , fRMSPlugin(DDSRMSPlugin::localhost)
        , fSession(std::make_shared<dds::tools_api::CSession>())
        , fDDSCustomCmd(fDDSService)
        , fId(to_string(fSession->create()))
        , fStopOnDestruction(false)
    {
        setenv("DDS_SESSION_ID", fId.c_str(), 1);

        fDDSService.subscribeOnError([](const dds::intercom_api::EErrorCode errorCode, const std::string& msg) {
            std::cerr << "DDS error, error code: " << errorCode << ", error message: " << msg << std::endl;
        });
    }

    explicit Impl(Id existing, DDSEnvironment env)
        : fEnv(std::move(env))
        , fRMSPlugin(DDSRMSPlugin::localhost)
        , fSession(std::make_shared<dds::tools_api::CSession>())
        , fDDSCustomCmd(fDDSService)
        , fId(std::move(existing))
        , fStopOnDestruction(false)
    {
        fSession->attach(fId);
        auto envId(std::getenv("DDS_SESSION_ID"));
        if (envId != nullptr && std::string(envId) != fId) {
            setenv("DDS_SESSION_ID", fId.c_str(), 1);
        }

        fDDSService.subscribeOnError([](const dds::intercom_api::EErrorCode errorCode, const std::string& msg) {
            std::cerr << "DDS error, error code: " << errorCode << ", error message: " << msg << std::endl;
        });
    }

    explicit Impl(std::shared_ptr<dds::tools_api::CSession> nativeSession, DDSEnv env)
        : fEnv(std::move(env))
        , fRMSPlugin(DDSRMSPlugin::localhost)
        , fSession(std::move(nativeSession))
        , fDDSCustomCmd(fDDSService)
        , fId(to_string(fSession->getSessionID()))
        , fStopOnDestruction(false)
    {
        auto envId(std::getenv("DDS_SESSION_ID"));
        if (envId != nullptr && std::string(envId) != fId) {
            setenv("DDS_SESSION_ID", fId.c_str(), 1);
        }

        // Sanity check
        if (!fSession->IsRunning()) {
            throw std::runtime_error("Given CSession must be running");
        }
    }

    ~Impl()
    {
        if (fStopOnDestruction) {
            fSession->shutdown();
        }
    }

    Impl() = delete;
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    struct Tag {};
    friend auto operator<<(std::ostream& os, Tag) -> std::ostream& { return os << "DDSSession"; }
    tools::InstanceLimiter<Tag, 1> fCount;

    DDSEnvironment fEnv;
    DDSRMSPlugin fRMSPlugin;
    Path fRMSConfig;
    std::shared_ptr<dds::tools_api::CSession> fSession;
    dds::intercom_api::CIntercomService fDDSService;
    dds::intercom_api::CCustomCmd fDDSCustomCmd;
    Id fId;
    bool fStopOnDestruction;
    mutable std::mutex fMtx;
    std::unordered_map<DDSChannel::Id, DDSTask::Id> fTaskIdByChannelIdMap;
};

DDSSession::DDSSession(DDSEnvironment env)
    : fImpl(std::make_shared<Impl>(std::move(env)))
{}

DDSSession::DDSSession(Id existing, DDSEnvironment env)
    : fImpl(std::make_shared<Impl>(std::move(existing), std::move(env)))
{}

DDSSession::DDSSession(std::shared_ptr<dds::tools_api::CSession> nativeSession, DDSEnv env)
    : fImpl(std::make_shared<Impl>(std::move(nativeSession), std::move(env)))
{}

auto DDSSession::GetEnv() const -> DDSEnvironment { return fImpl->fEnv; }

auto DDSSession::IsRunning() const -> bool { return fImpl->fSession->IsRunning(); }

auto DDSSession::GetId() const -> Id { return fImpl->fId; }

auto DDSSession::Stop() -> void { return fImpl->fSession->shutdown(); }

auto DDSSession::GetRMSPlugin() const -> DDSRMSPlugin { return fImpl->fRMSPlugin; }

auto DDSSession::SetRMSPlugin(DDSRMSPlugin plugin) -> void { fImpl->fRMSPlugin = plugin; }

auto DDSSession::GetRMSConfig() const -> Path { return fImpl->fRMSConfig; }

auto DDSSession::SetRMSConfig(Path configFile) const -> void
{
    fImpl->fRMSConfig = std::move(configFile);
}

auto DDSSession::IsStoppedOnDestruction() const -> bool { return fImpl->fStopOnDestruction; }

auto DDSSession::StopOnDestruction(bool stop) -> void { fImpl->fStopOnDestruction = stop; }

auto DDSSession::SubmitAgents(Quantity agents) -> void
{
    // Requesting to submit 0 agents is not meaningful
    assert(agents > 0);

    dds::tools_api::SSubmitRequestData submitInfo;
    submitInfo.m_rms = tools::ToString(GetRMSPlugin());
    submitInfo.m_instances = agents;
    submitInfo.m_config = GetRMSConfig().string();

    tools::Semaphore blocker;
    auto submitRequest = dds::tools_api::SSubmitRequest::makeRequest(submitInfo);
    submitRequest->setMessageCallback(
        [](const dds::tools_api::SMessageResponseData& message) { LOG(debug) << message; });
    submitRequest->setDoneCallback([&]() {
        LOG(debug) << agents << " Agents submitted";
        blocker.Signal();
    });

    fImpl->fSession->sendRequest<dds::tools_api::SSubmitRequest>(submitRequest);
    blocker.Wait();

    // perfect
    WaitForIdleAgents(agents);
}

auto DDSSession::RequestAgentInfo() -> AgentInfo
{
    dds::tools_api::SAgentInfoRequestData agentInfoInfo;
    tools::Semaphore blocker;
    AgentInfo info;
    auto agentInfoRequest = dds::tools_api::SAgentInfoRequest::makeRequest(agentInfoInfo);
    agentInfoRequest->setResponseCallback(
        [this, &info](const dds::tools_api::SAgentInfoResponseData& _response) {
            if (_response.m_index == 0) {
                info.activeAgentsCount = _response.m_activeAgentsCount;
                info.idleAgentsCount = _response.m_idleAgentsCount;
                info.executingAgentsCount = _response.m_executingAgentsCount;
                info.agents.reserve(_response.m_activeAgentsCount);
            }
            info.agents.emplace_back(*this, _response.m_agentInfo);
        });
    agentInfoRequest->setMessageCallback(
        [](const dds::tools_api::SMessageResponseData& _message) { LOG(debug) << _message; });
    agentInfoRequest->setDoneCallback([&]() { blocker.Signal(); });
    fImpl->fSession->sendRequest<dds::tools_api::SAgentInfoRequest>(agentInfoRequest);
    blocker.Wait();

    return info;
}

auto DDSSession::RequestCommanderInfo() -> CommanderInfo
{
    dds::tools_api::SCommanderInfoRequestData commanderInfoInfo;
    tools::Semaphore blocker;
    std::string error;
    auto commanderInfoRequest =
        dds::tools_api::SCommanderInfoRequest::makeRequest(commanderInfoInfo);
    CommanderInfo info;
    commanderInfoRequest->setResponseCallback(
        [&info](const dds::tools_api::SCommanderInfoResponseData& _response) {
            info.pid = _response.m_pid;
            info.activeTopologyName = _response.m_activeTopologyName;
        });
    commanderInfoRequest->setMessageCallback(
        [&](const dds::tools_api::SMessageResponseData& _message) {
            if (_message.m_severity == dds::intercom_api::EMsgSeverity::error) {
                error = _message.m_msg;
                blocker.Signal();
            } else {
                LOG(debug) << _message;
            }
        });
    commanderInfoRequest->setDoneCallback([&]() { blocker.Signal(); });
    fImpl->fSession->sendRequest<dds::tools_api::SCommanderInfoRequest>(commanderInfoRequest);
    blocker.Wait();

    if (!error.empty()) {
        throw std::runtime_error(error);
    }

    return info;
}

auto DDSSession::WaitForExecutingAgents(Quantity minCount) -> void
{
    auto info(RequestAgentInfo());
    int interval(8);
    while (info.executingAgentsCount < minCount) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        interval = std::min(256, interval * 2);
        info = RequestAgentInfo();
    }
}

auto DDSSession::WaitForIdleAgents(Quantity minCount) -> void
{
    auto info(RequestAgentInfo());
    int interval(8);
    while (info.idleAgentsCount < minCount) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        interval = std::min(256, interval * 2);
        info = RequestAgentInfo();
    }
}

auto DDSSession::ActivateTopology(DDSTopology topo) -> void
{
    dds::tools_api::STopologyRequestData topologyInfo;
    topologyInfo.m_updateType = dds::tools_api::STopologyRequestData::EUpdateType::ACTIVATE;
    topologyInfo.m_topologyFile = topo.GetTopoFile().string();

    tools::Semaphore blocker;
    auto topologyRequest = dds::tools_api::STopologyRequest::makeRequest(topologyInfo);
    topologyRequest->setMessageCallback(
        [](const dds::tools_api::SMessageResponseData& _message) { LOG(debug) << _message; });
    topologyRequest->setDoneCallback([&]() { blocker.Signal(); });
    fImpl->fSession->sendRequest<dds::tools_api::STopologyRequest>(topologyRequest);
    blocker.Wait();

    WaitForExecutingAgents(topo.GetNumRequiredAgents());
}

auto DDSSession::ActivateTopology(const Path& topoFile) -> void
{
    ActivateTopology(DDSTopology(topoFile, GetEnv()));
}

void DDSSession::StartDDSService() { fImpl->fDDSService.start(fImpl->fId); }

void DDSSession::SubscribeToCommands(std::function<void(const std::string& msg, const std::string& condition, uint64_t senderId)> cb)
{
    fImpl->fDDSCustomCmd.subscribe(cb);
    // fImpl->fDDSCustomCmd.subscribeOnReply([](const std::string& reply) {
    //     LOG(debug) << reply;
    // });
}

void DDSSession::UnsubscribeFromCommands()
{
    fImpl->fDDSCustomCmd.unsubscribe();
}

void DDSSession::SendCommand(const std::string& cmd) { fImpl->fDDSCustomCmd.send(cmd, ""); }

auto DDSSession::UpdateChannelToTaskAssociation(DDSChannel::Id channelId, DDSTask::Id taskId) -> void
{
    std::lock_guard<std::mutex> lk(fImpl->fMtx);
    fImpl->fTaskIdByChannelIdMap[channelId] = taskId;
}

auto DDSSession::GetTaskId(DDSChannel::Id channelId) const -> DDSTask::Id
{
    std::lock_guard<std::mutex> lk(fImpl->fMtx);
    return fImpl->fTaskIdByChannelIdMap.at(channelId);
}

auto operator<<(std::ostream& os, const DDSSession& session) -> std::ostream&
{
    return os << "$DDS_SESSION_ID: " << session.GetId();
}

auto DDSAgent::GetSession() const -> DDSSession
{
    return fSession;
}

auto DDSAgent::GetInfoStr() const -> std::string
{
    return fInfoStr;
}

auto operator<<(std::ostream& os, const DDSAgent& agent) -> std::ostream&
{
    return os << agent.GetInfoStr();
}

}   // namespace sdk
}   // namespace mq
}   // namespace fair

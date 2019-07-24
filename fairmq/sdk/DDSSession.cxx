/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DDSSession.h"

#include <fairmq/sdk/DDSEnvironment.h>
#include <fairmq/Tools.h>

#include <fairlogger/Logger.h>

#include <DDS/Tools.h>

#include <boost/uuid/uuid_io.hpp>

#include <cassert>
#include <cstdlib>
#include <sstream>
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
        , fDDSService()
        , fDDSCustomCmd(fDDSService)
        , fId(to_string(fSession.create()))
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
        , fDDSService()
        , fDDSCustomCmd(fDDSService)
        , fId(std::move(existing))
        , fStopOnDestruction(false)
    {
        fSession.attach(fId);
        std::string envId(std::getenv("DDS_SESSION_ID"));
        if (envId != fId) {
            setenv("DDS_SESSION_ID", fId.c_str(), 1);
        }

        fDDSService.subscribeOnError([](const dds::intercom_api::EErrorCode errorCode, const std::string& msg) {
            std::cerr << "DDS error, error code: " << errorCode << ", error message: " << msg << std::endl;
        });
    }

    ~Impl()
    {
        if (fStopOnDestruction) {
            fSession.shutdown();
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
    dds::tools_api::CSession fSession;
    dds::intercom_api::CIntercomService fDDSService;
    dds::intercom_api::CCustomCmd fDDSCustomCmd;
    Id fId;
    bool fStopOnDestruction;
};

DDSSession::DDSSession(DDSEnvironment env)
    : fImpl(std::make_shared<Impl>(env))
{}

DDSSession::DDSSession(Id existing, DDSEnvironment env)
    : fImpl(std::make_shared<Impl>(std::move(existing), env))
{}

auto DDSSession::GetEnv() const -> DDSEnvironment { return fImpl->fEnv; }

auto DDSSession::IsRunning() const -> bool { return fImpl->fSession.IsRunning(); }

auto DDSSession::GetId() const -> Id { return fImpl->fId; }

auto DDSSession::Stop() -> void { return fImpl->fSession.shutdown(); }

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

    fImpl->fSession.sendRequest<dds::tools_api::SSubmitRequest>(submitRequest);
    blocker.Wait();
}

auto DDSSession::RequestAgentInfo() -> void
{
    dds::tools_api::SAgentInfoRequestData agentInfoInfo;
    tools::Semaphore blocker;
    auto agentInfoRequest = dds::tools_api::SAgentInfoRequest::makeRequest(agentInfoInfo);
    agentInfoRequest->setResponseCallback(
        [&](const dds::tools_api::SAgentInfoResponseData& _response) {
            LOG(debug) << "agent: " << _response.m_index << "/" << _response.m_activeAgentsCount;
            LOG(debug) << "info: " << _response.m_agentInfo;
        });
    agentInfoRequest->setMessageCallback(
        [](const dds::tools_api::SMessageResponseData& _message) { LOG(debug) << _message; });
    agentInfoRequest->setDoneCallback([&]() { blocker.Signal(); });
    fImpl->fSession.sendRequest<dds::tools_api::SAgentInfoRequest>(agentInfoRequest);
    blocker.Wait();
}

auto DDSSession::RequestCommanderInfo() -> CommanderInfo
{
    dds::tools_api::SCommanderInfoRequestData commanderInfoInfo;
    tools::Semaphore blocker;
    auto commanderInfoRequest =
        dds::tools_api::SCommanderInfoRequest::makeRequest(commanderInfoInfo);
    CommanderInfo info;
    commanderInfoRequest->setResponseCallback(
        [&info](const dds::tools_api::SCommanderInfoResponseData& _response) {
            info.pid = _response.m_pid;
            info.idleAgentsCount = _response.m_idleAgentsCount;
        });
    commanderInfoRequest->setMessageCallback(
        [](const dds::tools_api::SMessageResponseData& _message) { LOG(debug) << _message; });
    commanderInfoRequest->setDoneCallback([&]() { blocker.Signal(); });
    fImpl->fSession.sendRequest<dds::tools_api::SCommanderInfoRequest>(commanderInfoRequest);
    blocker.Wait();

    return info;
}

auto DDSSession::WaitForIdleAgents(Quantity minCount) -> void
{
    auto info(RequestCommanderInfo());
    int interval(8);
    while (info.idleAgentsCount < minCount) {
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
        interval = std::min(256, interval * 2);
        info = RequestCommanderInfo();
    }
}

auto DDSSession::ActivateTopology(const Path& topologyFile) -> void
{
    dds::tools_api::STopologyRequestData topologyInfo;
    topologyInfo.m_updateType = dds::tools_api::STopologyRequestData::EUpdateType::ACTIVATE;
    topologyInfo.m_topologyFile = topologyFile.string();

    tools::Semaphore blocker;
    auto topologyRequest = dds::tools_api::STopologyRequest::makeRequest(topologyInfo);
    topologyRequest->setMessageCallback(
        [](const dds::tools_api::SMessageResponseData& _message) { LOG(debug) << _message; });
    topologyRequest->setDoneCallback([&]() { blocker.Signal(); });
    fImpl->fSession.sendRequest<dds::tools_api::STopologyRequest>(topologyRequest);
    blocker.Wait();
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

auto operator<<(std::ostream& os, const DDSSession& session) -> std::ostream&
{
    return os << "$DDS_SESSION_ID: " << session.GetId();
}

}   // namespace sdk
}   // namespace mq
}   // namespace fair

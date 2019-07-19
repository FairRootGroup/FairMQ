/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DDSSession.h"
#include "DDSEnvironment.h"

#include <DDS/Tools.h>
#include <boost/uuid/uuid_io.hpp>
#include <cassert>
#include <cstdlib>
#include <fairlogger/Logger.h>
#include <fairmq/Tools.h>
#include <sstream>
#include <stdlib.h>
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
    Impl(DDSEnvironment env, DDSRMSPlugin plugin)
        : fCount()
        , fEnv(std::move(env))
        , fDefaultPlugin(std::move(plugin))
        , fSession()
        , fId(to_string(fSession.create()))
    {
        setenv("DDS_SESSION_ID", fId.c_str(), 1);
    }

    Impl(DDSEnvironment env, DDSRMSPlugin plugin, Id existing_id)
        : fCount()
        , fEnv(std::move(env))
        , fDefaultPlugin(std::move(plugin))
        , fSession()
        , fId(std::move(existing_id))
    {
        fSession.attach(fId);
        std::string envId(std::getenv("DDS_SESSION_ID"));
        if (envId != fId) {
            setenv("DDS_SESSION_ID", fId.c_str(), 1);
        }
    }

    ~Impl()
    {
        fSession.shutdown();
    }
    struct Tag {};
    friend auto operator<<(std::ostream& os, Tag) -> std::ostream& { return os << "DDSSession"; }
    tools::InstanceLimiter<Tag, 1> fCount;

    const DDSEnvironment fEnv;
    const DDSRMSPlugin fDefaultPlugin;
    dds::tools_api::CSession fSession;
    const Id fId;
};

DDSSession::DDSSession(DDSEnvironment env, DDSRMSPlugin default_plugin)
: fImpl(std::make_shared<Impl>(std::move(env), std::move(default_plugin))) {}

DDSSession::DDSSession(DDSEnvironment env, Id existing_id)
: fImpl(std::make_shared<Impl>(std::move(env), DDSRMSPlugin::localhost, std::move(existing_id))) {}

DDSSession::DDSSession(DDSEnvironment env, DDSRMSPlugin default_plugin, Id existing_id)
: fImpl(std::make_shared<Impl>(std::move(env), std::move(default_plugin), std::move(existing_id))) {}

auto DDSSession::IsRunning() const -> bool { return fImpl->fSession.IsRunning(); }

auto DDSSession::GetId() const -> Id { return fImpl->fId; }

auto DDSSession::GetDefaultPlugin() const -> DDSRMSPlugin { return fImpl->fDefaultPlugin; }

auto DDSSession::SubmitAgents(Quantity agents) -> void
{
    SubmitAgents(agents, GetDefaultPlugin(), Path());
}

auto DDSSession::SubmitAgents(Quantity agents, DDSRMSPlugin plugin) -> void
{
    SubmitAgents(agents, plugin, Path());
}

auto DDSSession::SubmitAgents(Quantity agents, const Path& config) -> void
{
    SubmitAgents(agents, GetDefaultPlugin(), std::move(config));
}

auto DDSSession::SubmitAgents(Quantity agents, DDSRMSPlugin plugin, const Path& config) -> void
{
    // Requesting to submit 0 agents is not meaningful
    assert(agents > 0);
    // The config argument is required with all plugins except localhost
    if (plugin != DDSRMSPlugin::localhost) {
        assert(exists(config));
    }

    dds::tools_api::SSubmitRequestData submitInfo;
    submitInfo.m_rms = tools::ToString(plugin);
    submitInfo.m_instances = agents;
    submitInfo.m_config = config.string();

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

auto DDSSession::ActivateTopology(Path topologyFile) -> void
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

auto operator<<(std::ostream& os, DDSSession session) -> std::ostream&
{
    return os << "$DDS_SESSION_ID: " << session.GetId();
}

}   // namespace sdk
}   // namespace mq
}   // namespace fair

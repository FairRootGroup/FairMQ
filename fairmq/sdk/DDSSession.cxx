/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DDSSession.h"

#include <DDS/Tools.h>
#include <boost/process.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cassert>
#include <cstdlib>
#include <fairlogger/Logger.h>
#include <fairmq/Tools.h>
#include <fairmq/sdk/DDSAgent.h>
#include <fairmq/sdk/DDSEnvironment.h>
#include <fairmq/sdk/DDSTopology.h>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

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

    using namespace dds::tools_api;

    SSubmitRequestData submitInfo;
    submitInfo.m_rms = tools::ToString(GetRMSPlugin());
    submitInfo.m_instances = 1;
    submitInfo.m_slots = agents; // TODO new api: get slots from agents
    submitInfo.m_config = GetRMSConfig().string();

    tools::SharedSemaphore blocker;
    auto submitRequest = SSubmitRequest::makeRequest(submitInfo);
    submitRequest->setMessageCallback([](const SMessageResponseData& message){
        LOG(debug) << message.m_msg;
    });
    submitRequest->setDoneCallback([agents, blocker]() mutable {
        LOG(debug) << agents << " Agents submitted";
        blocker.Signal();
    });

    fImpl->fSession->sendRequest<SSubmitRequest>(submitRequest);
    blocker.Wait();

    WaitForIdleAgents(agents);
}

auto DDSSession::RequestAgentCount() -> AgentCount
{
    using namespace dds::tools_api;

    SAgentCountRequest::response_t res;
    fImpl->fSession->syncSendRequest<SAgentCountRequest>(SAgentCountRequest::request_t(), res);

    AgentCount count;
    count.active = res.m_activeSlotsCount;
    count.idle = res.m_idleSlotsCount;
    count.executing = res.m_executingSlotsCount;

    return count;
}

auto DDSSession::RequestAgentInfo() -> std::vector<DDSAgent>
{
    using namespace dds::tools_api;

    SAgentInfoRequest::responseVector_t res;
    fImpl->fSession->syncSendRequest<SAgentInfoRequest>(SAgentInfoRequest::request_t(), res);

    std::vector<DDSAgent> agentInfo;
    agentInfo.reserve(res.size());
    for (const auto& a : res) {
        agentInfo.emplace_back(
            *this,
            a.m_agentID,
            a.m_agentPid,
            a.m_DDSPath,
            a.m_host,
            a.m_startUpTime,
            a.m_username
            // a.m_nSlots
        );
    }

    return agentInfo;
}

auto DDSSession::RequestTaskInfo() -> std::vector<DDSTask>
{
    using namespace dds::tools_api;

    SAgentInfoRequest::responseVector_t res;
    fImpl->fSession->syncSendRequest<SAgentInfoRequest>(SAgentInfoRequest::request_t(), res);

    std::vector<DDSTask> taskInfo;
    taskInfo.reserve(res.size());
    for (auto& a : res) {
        //taskInfo.emplace_back(a.m_taskID, 0);
        taskInfo.emplace_back(0, 0);
    }

    return taskInfo;
}

auto DDSSession::RequestCommanderInfo() -> CommanderInfo
{
    using namespace dds::tools_api;

    SCommanderInfoRequestData commanderInfo;
    tools::SharedSemaphore blocker;
    std::string error;
    auto commanderInfoRequest = SCommanderInfoRequest::makeRequest(commanderInfo);
    CommanderInfo info;
    commanderInfoRequest->setResponseCallback([&info](const SCommanderInfoResponseData& _response) {
        info.pid = _response.m_pid;
        info.activeTopologyName = _response.m_activeTopologyName;
    });
    commanderInfoRequest->setMessageCallback([&](const SMessageResponseData& _message) {
        if (_message.m_severity == dds::intercom_api::EMsgSeverity::error) {
            error = _message.m_msg;
            blocker.Signal();
        } else {
            LOG(debug) << _message.m_msg;
        }
    });
    commanderInfoRequest->setDoneCallback([blocker]() mutable { blocker.Signal(); });
    fImpl->fSession->sendRequest<SCommanderInfoRequest>(commanderInfoRequest);
    blocker.Wait();

    if (!error.empty()) {
        throw std::runtime_error(error);
    }

    return info;
}

auto DDSSession::WaitForExecutingAgents(Quantity minCount) -> void
{
    auto count(RequestAgentCount());
    int interval(8);
    while (count.executing < minCount) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        interval = std::min(256, interval * 2);
        count = RequestAgentCount();
    }
}

auto DDSSession::WaitForIdleAgents(Quantity minCount) -> void
{
    auto count(RequestAgentCount());
    int interval(8);
    while (count.idle < minCount) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        interval = std::min(256, interval * 2);
        count = RequestAgentCount();
    }
}

auto DDSSession::ActivateTopology(DDSTopology topo) -> void
{
    using namespace dds::tools_api;

    STopologyRequestData topologyInfo;
    topologyInfo.m_updateType = STopologyRequestData::EUpdateType::ACTIVATE;
    topologyInfo.m_topologyFile = topo.GetTopoFile().string();

    tools::SharedSemaphore blocker;
    auto topologyRequest = STopologyRequest::makeRequest(topologyInfo);
    topologyRequest->setMessageCallback([](const SMessageResponseData& _message) {
        LOG(debug) << _message.m_msg;
    });
    topologyRequest->setDoneCallback([blocker]() mutable { blocker.Signal(); });
    fImpl->fSession->sendRequest<STopologyRequest>(topologyRequest);
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

void DDSSession::SendCommand(const std::string& cmd, DDSChannel::Id recipient)
{
    fImpl->fDDSCustomCmd.send(cmd, std::to_string(recipient));
}

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

auto getMostRecentRunningDDSSession(DDSEnv env) -> DDSSession
{
    boost::process::ipstream pipeStream;
    boost::process::child c("dds-session list all", boost::process::std_out > pipeStream);
    std::string lastLine;
    std::string currentLine;

    while (pipeStream && std::getline(pipeStream, currentLine) && !currentLine.empty()) {
        lastLine = currentLine;
    }
    c.wait();
    std::string sessionId;

    if (!lastLine.empty()) {
        std::vector<std::string> words;
        std::istringstream iss(lastLine);
        for (std::string s; iss >> s;) {
            if (s != "*") {
                words.push_back(s);
            }
        }
        if (words.back() == "RUNNING") {
            sessionId = words.front();
        }
    }

    if (sessionId.empty()) {
        throw std::runtime_error("could not find most recent DDS session");
    }

    return DDSSession(DDSSession::Id(sessionId), std::move(env));
}

}   // namespace sdk
}   // namespace mq
}   // namespace fair

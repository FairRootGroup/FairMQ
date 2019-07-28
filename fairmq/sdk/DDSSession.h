/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_DDSSESSION_H
#define FAIR_MQ_SDK_DDSSESSION_H

#include <fairmq/sdk/DDSEnvironment.h>
#include <fairmq/sdk/DDSInfo.h>

#include <boost/filesystem.hpp>

#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <functional>

namespace fair {
namespace mq {
namespace sdk {

class DDSEnvironment;

/**
 * @enum DDSRMSPlugin DDSSession.h <fairmq/sdk/DDSSession.h>
 * @brief Supported DDS resource management system plugins
 */
enum class DDSRMSPlugin
{
    localhost,
    ssh
};
auto operator<<(std::ostream& os, DDSRMSPlugin plugin) -> std::ostream&;
auto operator>>(std::istream& is, DDSRMSPlugin& plugin) -> std::istream&;

class DDSTopology;
class DDSAgent;

class DDSTask
{
  public:
    using Id = std::uint64_t;
};

class DDSChannel
{
  public:
    using Id = std::uint64_t;
};

/**
 * @class DDSSession DDSSession.h <fairmq/sdk/DDSSession.h>
 * @brief Represents a DDS session
 */
class DDSSession
{
  public:
    using Id = std::string;
    using Quantity = std::uint32_t;
    using Path = boost::filesystem::path;

    explicit DDSSession(DDSEnvironment env = DDSEnvironment());
    explicit DDSSession(Id existing, DDSEnvironment env = DDSEnvironment());

    /// @brief Construct with already existing native DDS API objects
    /// @param nativeSession Existing and initialized CSession (either via create() or attach())
    /// @param env Optional DDSEnv
    explicit DDSSession(std::shared_ptr<dds::tools_api::CSession> nativeSession, DDSEnv env = {});

    auto GetEnv() const -> DDSEnvironment;
    auto GetId() const -> Id;
    auto GetRMSPlugin() const -> DDSRMSPlugin;
    auto SetRMSPlugin(DDSRMSPlugin) -> void;
    auto GetRMSConfig() const -> Path;
    auto SetRMSConfig(Path) const -> void;
    auto IsStoppedOnDestruction() const -> bool;
    auto StopOnDestruction(bool stop = true) -> void;
    auto IsRunning() const -> bool;
    auto SubmitAgents(Quantity agents) -> void;
    struct AgentInfo {
        Quantity idleAgentsCount = 0;
        Quantity activeAgentsCount = 0;
        Quantity executingAgentsCount = 0;
        std::vector<DDSAgent> agents;
    };
    auto RequestAgentInfo() -> AgentInfo;
    struct CommanderInfo {
        int pid = -1;
        std::string activeTopologyName;
    };
    auto RequestCommanderInfo() -> CommanderInfo;
    auto WaitForIdleAgents(Quantity) -> void;
    auto WaitForOnlyIdleAgents() -> void;
    auto WaitForExecutingAgents(Quantity) -> void;
    auto ActivateTopology(const Path& topoFile) -> void;
    auto ActivateTopology(DDSTopology) -> void;
    auto Stop() -> void;

    void StartDDSService();
    void SubscribeToCommands(std::function<void(const std::string& msg, const std::string& condition, uint64_t senderId)>);
    void UnsubscribeFromCommands();
    void SendCommand(const std::string&);
    auto UpdateChannelToTaskAssociation(DDSChannel::Id, DDSTask::Id) -> void;
    auto GetTaskId(DDSChannel::Id) const -> DDSTask::Id;

    friend auto operator<<(std::ostream& os, const DDSSession& session) -> std::ostream&;

  private:
    struct Impl;
    std::shared_ptr<Impl> fImpl;
};

/**
 * @class DDSAgent DDSSession.h <fairmq/sdk/DDSSession.h>
 * @brief Represents a DDS agent
 */
class DDSAgent
{
  public:
    explicit DDSAgent(DDSSession session, std::string infostr)
        : fInfoStr(std::move(infostr))
        , fSession(std::move(session))
    {}

    auto GetSession() const -> DDSSession;
    auto GetInfoStr() const -> std::string;

    friend auto operator<<(std::ostream& os, const DDSAgent& plugin) -> std::ostream&;

  private:
    std::string fInfoStr;
    DDSSession fSession;
};
}   // namespace sdk
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SDK_DDSSESSION_H */

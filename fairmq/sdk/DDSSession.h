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
    auto RequestAgentInfo() -> void;
    auto RequestCommanderInfo() -> void;
    auto ActivateTopology(const Path& topologyFile) -> void;
    auto Stop() -> void;

    void StartDDSService();
    void SubscribeToCommands(std::function<void(const std::string& msg, const std::string& condition, uint64_t senderId)>);
    void UnsubscribeFromCommands();
    void SendCommand(const std::string&);

    friend auto operator<<(std::ostream& os, const DDSSession& session) -> std::ostream&;
  private:
    struct Impl;
    std::shared_ptr<Impl> fImpl;
};

}   // namespace sdk
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SDK_DDSSESSION_H */

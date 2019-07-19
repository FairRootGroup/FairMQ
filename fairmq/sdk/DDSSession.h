/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_DDSSESSION_H
#define FAIR_MQ_SDK_DDSSESSION_H

#include <boost/filesystem.hpp>
#include <cstdint>
#include <fairmq/sdk/DDSInfo.h>
#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>

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

    DDSSession() = delete;
    explicit DDSSession(DDSEnvironment env, DDSRMSPlugin default_plugin = DDSRMSPlugin::localhost);
    explicit DDSSession(DDSEnvironment env, Id existing_id);
    explicit DDSSession(DDSEnvironment env, DDSRMSPlugin default_plugin, Id existing_id);

    auto GetId() const -> Id;
    auto GetDefaultPlugin() const -> DDSRMSPlugin;
    auto IsRunning() const -> bool;
    auto SubmitAgents(Quantity agents) -> void;
    auto SubmitAgents(Quantity agents, DDSRMSPlugin plugin) -> void;
    auto SubmitAgents(Quantity agents, DDSRMSPlugin plugin, const Path& config) -> void;
    auto SubmitAgents(Quantity agents, const Path& config) -> void;
    auto RequestAgentInfo() -> void;
    auto ActivateTopology(Path topologyFile) -> void;
    auto Stop() -> void;

    friend auto operator<<(std::ostream& os, DDSSession session) -> std::ostream&;
  private:
    struct Impl;
    std::shared_ptr<Impl> fImpl;
};

}   // namespace sdk
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SDK_DDSSESSION_H */

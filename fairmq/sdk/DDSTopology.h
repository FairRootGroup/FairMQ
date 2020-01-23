/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_DDSTOPOLOGY_H
#define FAIR_MQ_SDK_DDSTOPOLOGY_H

#include <boost/filesystem.hpp>
#include <fairmq/sdk/DDSCollection.h>
#include <fairmq/sdk/DDSEnvironment.h>
#include <fairmq/sdk/DDSInfo.h>
#include <fairmq/sdk/DDSTask.h>
#include <memory>
#include <string>
#include <vector>

namespace fair {
namespace mq {
namespace sdk {

/**
 * @class DDSTopology DDSTopology.h <fairmq/sdk/DDSTopology.h>
 * @brief Represents a DDS topology
 */
class DDSTopology
{
  public:
    using Path = boost::filesystem::path;

    DDSTopology() = delete;

    /// @brief Construct from file
    /// @param topoFile DDS topology xml file
    /// @param env DDS environment
    explicit DDSTopology(Path topoFile, DDSEnvironment env = DDSEnvironment());

    /// @brief Construct with already existing native DDS API objects
    /// @param nativeTopology Existing and initialized CTopology
    /// @param env Optional DDSEnv
    explicit DDSTopology(dds::topology_api::CTopology nativeTopology, DDSEnv env = {});

    /// @brief Get associated DDS environment
    auto GetEnv() const -> DDSEnvironment;

    /// @brief Get path to DDS topology xml, if it is known
    /// @throw std::runtime_error
    auto GetTopoFile() const -> Path;

    /// @brief Get number of required agents for this topology
    auto GetNumRequiredAgents() const -> int;

    /// @brief Get list of tasks in this topology
    auto GetTasks() const -> std::vector<DDSTask>;

    /// @brief Get list of tasks matching the provided topology path
    auto GetTasksMatchingPath(const std::string&) const -> std::vector<DDSTask>;

    /// @brief Get list of tasks in this topology
    auto GetCollections() const -> std::vector<DDSCollection>;

    /// @brief Get the name of the topology
    auto GetName() const -> std::string;

    friend auto operator<<(std::ostream&, const DDSTopology&) -> std::ostream&;

  private:
    struct Impl;
    std::shared_ptr<Impl> fImpl;
};

using DDSTopo = DDSTopology;

}   // namespace sdk
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SDK_DDSTOPOLOGY_H */

/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_DDSTOPOLOGY_H
#define FAIR_MQ_SDK_DDSTOPOLOGY_H

#include <fairmq/sdk/DDSInfo.h>
#include <memory>
#include <string>

namespace dds {
namespace topology_api {

class CTopology;

}   // namespace topology_api
}   // namespace dds

namespace fair {
namespace mq {
namespace sdk {

/**
 * @class DDSTopology DDSTopology.h <fairmq/sdk/DDSTopology.h>
 * @brief Represents a DDS topology
 */
class DDSSession
{
  public:
    using CSessionPtr = std::shared_ptr<dds::tools_api::CSession>;

    explicit DDSSession();
    explicit DDSSession(std::string existing_session_id);

    auto GetId() const -> const std::string&;
    auto IsRunning() const -> bool;
  private:
    CSessionPtr fSession;
    const std::string fId;
};

auto LoadDDSEnv(const std::string& config_home = "", const std::string& prefix = DDSInstallPrefix)
    -> void;

}   // namespace sdk
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SDK_DDSTOPOLOGY_H */

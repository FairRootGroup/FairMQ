/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_DDSENVIRONMENT_H
#define FAIR_MQ_SDK_DDSENVIRONMENT_H

#include <boost/filesystem.hpp>
#include <memory>
#include <ostream>

namespace fair {
namespace mq {
namespace sdk {

/**
 * @class DDSEnvironment DDSSession.h <fairmq/sdk/DDSSession.h>
 * @brief Sets up the DDS environment (object helper)
 */
class DDSEnvironment
{
  public:
    using Path = boost::filesystem::path;

    DDSEnvironment();
    explicit DDSEnvironment(Path);

    auto GetLocation() const -> Path;
    auto GetConfigHome() const -> Path;

    friend auto operator<<(std::ostream& os, DDSEnvironment env) -> std::ostream&;
  private:
    struct Impl;
    std::shared_ptr<Impl> fImpl;
};

using DDSEnv = DDSEnvironment;

}   // namespace sdk
}   // namespace mq
}   // namespace fair

#endif /* FAIR_MQ_SDK_DDSENVIRONMENT_H */

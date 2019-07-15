/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DDSEnvironment.h"

#include <DDS/Tools.h>
#include <cstdlib>
#include <fairmq/Tools.h>
#include <sstream>
#include <stdlib.h>
#include <utility>

namespace fair {
namespace mq {
namespace sdk {

// TODO https://github.com/FairRootGroup/DDS/issues/224
auto LoadDDSEnv(const boost::filesystem::path& config_home, const boost::filesystem::path& prefix)
    -> void
{
    setenv("DDS_LOCATION", prefix.c_str(), 1);
    if (!config_home.empty()) {
        setenv("HOME", config_home.c_str(), 1);
    }
    std::string path(std::getenv("PATH"));
    path = DDSExecutableDir + std::string(":") + path;
    setenv("PATH", path.c_str(), 1);
    std::istringstream cmd;
    cmd.str("DDS_CFG=`dds-user-defaults --ignore-default-sid -p`\n"
            "if [ -z \"$DDS_CFG\" ]; then\n"
            "  dds-user-defaults --ignore-default-sid -d -c \"$HOME/.DDS/DDS.cfg\"\n"
            "fi");
    std::system(cmd.str().c_str());
}

struct DDSEnvironment::Impl
{
    Impl(Path config_home, Path prefix)
        : fConfigHome(std::move(config_home))
        , fInstallPrefix(std::move(prefix))
    {
        LoadDDSEnv(fConfigHome, fInstallPrefix);
        if (fConfigHome.empty()) {
            fConfigHome = std::getenv("HOME");
        }
    }

    Path fConfigHome;
    Path fInstallPrefix;
};

DDSEnvironment::DDSEnvironment(Path config_home, Path prefix)
    : fImpl(std::make_shared<Impl>(std::move(config_home), std::move(prefix)))
{}

auto DDSEnvironment::GetConfigHome() const -> Path { return fImpl->fConfigHome; }

auto DDSEnvironment::GetInstallPrefix() const -> Path { return fImpl->fInstallPrefix; }

auto operator<<(std::ostream& os, DDSEnvironment env) -> std::ostream&
{
    return os << "$DDS_LOCATION: " << env.GetInstallPrefix() << ", "
              << "$DDS_CONFIG_HOME: " << env.GetConfigHome() / DDSEnvironment::Path(".DDS");
}

}   // namespace sdk
}   // namespace mq
}   // namespace fair

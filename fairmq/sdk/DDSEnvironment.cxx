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
auto LoadDDSEnv(const boost::filesystem::path& config_home)
    -> void
{
    setenv("DDS_LOCATION", DDSInstallPrefix.c_str(), 1);
    if (!config_home.empty()) {
        setenv("HOME", config_home.c_str(), 1);
    }
    std::string path(std::getenv("PATH"));
    path = DDSExecutableDir + std::string(":") + path;
    setenv("PATH", path.c_str(), 1);

#ifndef __APPLE__
    std::string ldVar("LD_LIBRARY_PATH");
    std::string ld(std::getenv(ldVar.c_str()));
    ld = DDSLibraryDir + std::string(":") + ld;
    setenv(ldVar.c_str(), ld.c_str(), 1);
#endif

    std::istringstream cmd;
    cmd.str("DDS_CFG=`dds-user-defaults --ignore-default-sid -p`\n"
            "if [ -z \"$DDS_CFG\" ]; then\n"
            "  mkdir -p \"$HOME/.DDS\"\n"
            "  dds-user-defaults --ignore-default-sid -d -c \"$HOME/.DDS/DDS.cfg\"\n"
            "fi");
    std::system(cmd.str().c_str());
}

struct DDSEnvironment::Impl
{
    explicit Impl(Path config_home)
        : fCount()
        , fConfigHome(std::move(config_home))
    {
        LoadDDSEnv(fConfigHome);
        if (fConfigHome.empty()) {
            fConfigHome = std::getenv("HOME");
        }
    }

    struct Tag {};
    friend auto operator<<(std::ostream& os, Tag) -> std::ostream& { return os << "DDSEnvironment"; }
    tools::InstanceLimiter<Tag, 1> fCount;

    Path fConfigHome;
};

DDSEnvironment::DDSEnvironment(Path config_home)
    : fImpl(std::make_shared<Impl>(std::move(config_home)))
{}

auto DDSEnvironment::GetConfigHome() const -> Path { return fImpl->fConfigHome; }

auto operator<<(std::ostream& os, DDSEnvironment env) -> std::ostream&
{
    return os << "$DDS_LOCATION: " << DDSInstallPrefix << ", "
              << "$DDS_CONFIG_HOME: " << env.GetConfigHome() / DDSEnvironment::Path(".DDS");
}

}   // namespace sdk
}   // namespace mq
}   // namespace fair

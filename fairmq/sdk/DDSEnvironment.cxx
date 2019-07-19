/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DDSEnvironment.h"

#include <fairmq/Tools.h>
#include <fairmq/sdk/DDSInfo.h>

#include <fairlogger/Logger.h>

#include <DDS/Tools.h>
#include <DDS/dds_intercom.h>

#include <cstdlib>
#include <sstream>
#include <utility>

namespace fair {
namespace mq {
namespace sdk {

struct DDSEnvironment::Impl
{
    explicit Impl(Path configHome)
        : fLocation(DDSInstallPrefix)
        , fConfigHome(std::move(configHome))
    {
        SetupLocation();
        SetupDynamicLoader();
        SetupPath();
        SetupConfigHome();
    }

    auto SetupLocation() -> void
    {
        std::string location(GetEnv("DDS_LOCATION"));
        if (location != DDSInstallPrefix) {
            if (location.empty()) {
                setenv("DDS_LOCATION", DDSInstallPrefix.c_str(), 1);
            } else {
                LOG(debug) << "$DDS_LOCATION appears to point to a different installation than this"
                           << "program was linked against. Things might still work out, so not"
                           << "touching it.";
                fLocation = location;
            }
        }
    }

    auto SetupConfigHome() -> void
    {
        if (fConfigHome.empty()) {
            fConfigHome = GetEnv("HOME");
        } else {
            setenv("HOME", fConfigHome.c_str(), 1);
        }

        std::istringstream cmd;
        cmd.str("DDS_CFG=`dds-user-defaults --ignore-default-sid -p`\n"
                "if [ -z \"$DDS_CFG\" ]; then\n"
                "  mkdir -p \"$HOME/.DDS\"\n"
                "  dds-user-defaults --ignore-default-sid -d -c \"$HOME/.DDS/DDS.cfg\"\n"
                "fi");
        std::system(cmd.str().c_str());
    }

    auto SetupPath() -> void
    {
        std::string path(GetEnv("PATH"));
        Path ddsExecDir = (fLocation == DDSInstallPrefix) ? DDSExecutableDir : fLocation / Path("bin");
        path = ddsExecDir.string() + std::string(":") + path;
        setenv("PATH", path.c_str(), 1);
    }

    auto SetupDynamicLoader() -> void
    {
#ifdef __APPLE__
        std::string ldVar("DYLD_LIBRARY_PATH");
#else
        std::string ldVar("LD_LIBRARY_PATH");
#endif
        std::string ld(GetEnv(ldVar));
        Path ddsLibDir = (fLocation == DDSInstallPrefix) ? DDSLibraryDir : fLocation / Path("lib");
        ld = ddsLibDir.string() + std::string(":") + ld;
        setenv(ldVar.c_str(), ld.c_str(), 1);
    }

    auto GetEnv(const std::string& key) -> std::string
    {
        auto value = std::getenv(key.c_str());
        if (value) {
            return {value};
        }
        return {};
    }

    struct Tag {};
    friend auto operator<<(std::ostream& os, Tag) -> std::ostream& { return os << "DDSEnvironment"; }
    tools::InstanceLimiter<Tag, 1> fCount;

    Path fLocation;
    Path fConfigHome;
};

DDSEnvironment::DDSEnvironment()
    : DDSEnvironment(Path())
{}

DDSEnvironment::DDSEnvironment(Path configHome)
    : fImpl(std::make_shared<Impl>(std::move(configHome)))
{}

auto DDSEnvironment::GetLocation() const -> Path { return fImpl->fLocation; }

auto DDSEnvironment::GetConfigHome() const -> Path { return fImpl->fConfigHome; }

auto operator<<(std::ostream& os, DDSEnvironment env) -> std::ostream&
{
    return os << "$DDS_LOCATION: " << env.GetLocation() << ", "
              << "$DDS_CONFIG_HOME: " << env.GetConfigHome() / DDSEnvironment::Path(".DDS");
}

}   // namespace sdk
}   // namespace mq
}   // namespace fair

/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DDSEnvironment.h"

#include <cstdlib>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <dds/dds.h>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <fairlogger/Logger.h>
#include <fairmq/tools/InstanceLimit.h>
#include <fairmq/sdk/DDSInfo.h>
#include <sstream>
#include <utility>

namespace fair::mq::sdk
{

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

        std::stringstream cmd;
#ifdef __APPLE__
        // On macOS System Integrity Protection might filter out the DYLD_LIBRARY_PATH, so we pass it
        // through explicitely here.
        cmd << "export " << fgLdVar << "=" << GetEnv(fgLdVar) << "\n";
#endif
        cmd << "DDS_CFG=`dds-user-defaults --ignore-default-sid -p`\n"
               "if [ -z \"$DDS_CFG\" ]; then\n"
               "  mkdir -p \"$HOME/.DDS\"\n"
               "  dds-user-defaults --ignore-default-sid -d -c \"$HOME/.DDS/DDS.cfg\"\n"
               "fi\n";
        auto rc(std::system(cmd.str().c_str()));
        if (rc != 0) {
            LOG(warn) << "DDSEnvironment::SetupConfigHome failed";
        }
    }

    auto SetupPath() -> void
    {
        std::string path(GetEnv("PATH"));
        Path ddsExecDir = (fLocation == DDSInstallPrefix) ? DDSExecutableDir : fLocation / Path("bin");
        path = ddsExecDir.string() + std::string(":") + path;
        setenv("PATH", path.c_str(), 1);
    }

    auto GenerateDDSLibDir() const -> Path
    {
        return {(fLocation == DDSInstallPrefix) ? DDSLibraryDir : fLocation / Path("lib")};
    }

    auto SetupDynamicLoader() -> void
    {
        std::string ld(GetEnv(fgLdVar));
        if (ld.empty()) {
            ld = GenerateDDSLibDir().string();
        } else {
            ld = GenerateDDSLibDir().string() + std::string(":") + ld;
        }
        setenv(fgLdVar.c_str(), ld.c_str(), 1);
    }

    auto GetEnv(const std::string& key) const -> std::string
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
#ifdef __APPLE__
    std::string const fgLdVar = "DYLD_LIBRARY_PATH";
#else
    std::string const fgLdVar = "LD_LIBRARY_PATH";
#endif
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

} // namespace fair::mq::sdk

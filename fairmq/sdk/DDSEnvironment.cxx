/********************************************************************************
 * Copyright (C) 2019-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
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
        SetupPath();
        SetupConfigHome();
    }

    auto SetupConfigHome() -> void
    {
        if (fConfigHome.empty()) {
            fConfigHome = GetEnv("HOME");
        } else {
            setenv("HOME", fConfigHome.c_str(), 1);
        }
    }

    auto SetupPath() -> void
    {
        std::string path(GetEnv("PATH"));
        Path ddsExecDir = (fLocation == DDSInstallPrefix) ? DDSExecutableDir : fLocation / Path("bin");
        path = ddsExecDir.string() + std::string(":") + path;
        setenv("PATH", path.c_str(), 1);
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

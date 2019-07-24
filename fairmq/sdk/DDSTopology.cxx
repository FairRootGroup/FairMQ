/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DDSTopology.h"

#include <fairmq/sdk/DDSEnvironment.h>
#include <fairmq/Tools.h>

#include <fairlogger/Logger.h>

#include <DDS/Topology.h>

#include <sstream>
#include <stdexcept>
#include <utility>
#include <memory>

namespace fair {
namespace mq {
namespace sdk {

struct DDSTopology::Impl
{
    explicit Impl(Path topoFile, DDSEnvironment env)
        : fEnv(std::move(env))
        , fTopoFile(std::move(topoFile))
        , fTopo(nullptr)
    {
        LOG(warn) << topoFile;
    }

    DDSEnvironment fEnv;
    Path fTopoFile;
    std::unique_ptr<dds::topology_api::CTopology> fTopo;
};

DDSTopology::DDSTopology(Path topoFile, DDSEnvironment env)
    : fImpl(std::make_shared<Impl>(std::move(topoFile), std::move(env)))
{}

auto DDSTopology::GetEnv() const -> DDSEnvironment { return fImpl->fEnv; }

auto DDSTopology::GetTopoFile() const -> Path
{
    auto file(fImpl->fTopoFile);
    if (file.string().empty()) {
        throw std::runtime_error("DDS topology xml spec file unknown");
    }
    return file;
}

void DDSTopology::CreateTopology(Path topoFile)
{
    fImpl->fTopo = tools::make_unique<dds::topology_api::CTopology>(fImpl->fTopoFile.c_str());
}

int DDSTopology::GetNumRequiredAgents()
{
    return fImpl->fTopo->getRequiredNofAgents();
}

std::vector<uint64_t> DDSTopology::GetDeviceList()
{
    std::vector<uint64_t> taskIDs;
    taskIDs.reserve(fImpl->fTopo->getRequiredNofAgents());

    // TODO make sure returned tasks are actually devices
    dds::topology_api::STopoRuntimeTask::FilterIteratorPair_t taskIt = fImpl->fTopo->getRuntimeTaskIterator([](const dds::topology_api::STopoRuntimeTask::FilterIterator_t::value_type& value) -> bool {
        return true;
    });

    for (auto& it = taskIt.first; it != taskIt.second; ++it) {
        LOG(debug) << "Found task " << it->first << " : " << "Path: " << it->second.m_task->getPath() << "Name: " << it->second.m_task->getName();
        taskIDs.push_back(it->first);
    }

    return taskIDs;
}

auto DDSTopology::GetName() const -> std::string { return fImpl->fTopo.getName(); }

auto operator<<(std::ostream& os, const DDSTopology& t) -> std::ostream&
try {
    return os << "DDS topology: " << t.GetName() << " (loaded from " << t.GetTopoFile() << ")";
} catch (std::runtime_error&) {
    return os << "DDS topology: " << t.GetName();
}

}   // namespace sdk
}   // namespace mq
}   // namespace fair

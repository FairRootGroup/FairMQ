/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "DDSTopology.h"

#include <boost/range/iterator_range.hpp>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <dds/dds.h>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <fairlogger/Logger.h>
#include <fairmq/sdk/DDSEnvironment.h>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace fair::mq::sdk
{

struct DDSTopology::Impl
{
    explicit Impl(Path topoFile, DDSEnvironment env)
        : fEnv(std::move(env))
        , fTopoFile(std::move(topoFile))
        , fTopo(fTopoFile.string())
    {}

    explicit Impl(dds::topology_api::CTopology nativeTopology, DDSEnvironment env)
        : fEnv(std::move(env))
        , fTopo(std::move(nativeTopology))
    {}

    DDSEnvironment fEnv;
    Path fTopoFile;
    dds::topology_api::CTopology fTopo;
};

DDSTopology::DDSTopology(Path topoFile, DDSEnvironment env)
    : fImpl(std::make_shared<Impl>(std::move(topoFile), std::move(env)))
{}

DDSTopology::DDSTopology(dds::topology_api::CTopology nativeTopology, DDSEnv env)
    : fImpl(std::make_shared<Impl>(std::move(nativeTopology), std::move(env)))
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

auto DDSTopology::GetNumRequiredAgents() const -> std::size_t
{
    return fImpl->fTopo.getRequiredNofAgents();
}

auto DDSTopology::GetTasks(const std::string& path /* = "" */) const -> std::vector<DDSTask>
{
    std::vector<DDSTask> list;

    dds::topology_api::STopoRuntimeTask::FilterIteratorPair_t itPair;
    if (path.empty()) {
        itPair = fImpl->fTopo.getRuntimeTaskIterator(nullptr); // passing nullptr will get all tasks
    } else {
        itPair = fImpl->fTopo.getRuntimeTaskIteratorMatchingPath(path);
    }
    auto tasks = boost::make_iterator_range(itPair.first, itPair.second);

    for (const auto& task : tasks) {
        // LOG(debug) << "Found task with id: " << task.first << ", "
        //            << "Path: " << task.second.m_taskPath << ", "
        //            << "Collection id: " << task.second.m_taskCollectionId << ", "
        //            << "Name: " << task.second.m_task->getName() << "_" << task.second.m_taskIndex;
        list.emplace_back(task.first, task.second.m_taskCollectionId);
    }

    return list;
}

auto DDSTopology::GetCollections() const -> std::vector<DDSCollection>
{
    std::vector<DDSCollection> list;

    auto itPair = fImpl->fTopo.getRuntimeCollectionIterator(nullptr); // passing nullptr will get all collections
    auto collections = boost::make_iterator_range(itPair.first, itPair.second);

    for (const auto& c : collections) {
        LOG(debug) << "Found collection with id: " << c.first << ", "
                   << "Index: " << c.second.m_collectionIndex << ", "
                   << "Path: " << c.second.m_collectionPath;
        list.emplace_back(c.first);
    }

    return list;
}

auto DDSTopology::GetName() const -> std::string { return fImpl->fTopo.getName(); }

auto operator<<(std::ostream& os, const DDSTopology& t) -> std::ostream&
try {
    return os << "DDS topology: " << t.GetName() << " (loaded from " << t.GetTopoFile() << ")";
} catch (std::runtime_error&) {
    return os << "DDS topology: " << t.GetName();
}

} // namespace fair::mq::sdk

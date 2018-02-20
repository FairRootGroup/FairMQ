/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/shmem/Manager.h>
#include <fairmq/shmem/Common.h>

namespace fair
{
namespace mq
{
namespace shmem
{

using namespace std;
namespace bipc = boost::interprocess;

std::unordered_map<uint64_t, Region> Manager::fRegions;

Manager::Manager(const string& name, size_t size)
    : fSessionName(name)
    , fSegmentName("fmq_shm_" + fSessionName + "_main")
    , fManagementSegmentName("fmq_shm_" + fSessionName + "_management")
    , fSegment(bipc::open_or_create, fSegmentName.c_str(), size)
    , fManagementSegment(bipc::open_or_create, fManagementSegmentName.c_str(), 65536)
{}

bipc::managed_shared_memory& Manager::Segment()
{
    return fSegment;
}

void Manager::Interrupt()
{
}

void Manager::Resume()
{
    // close remote regions before processing new transfers
    for (auto it = fRegions.begin(); it != fRegions.end(); /**/)
    {
        if (it->second.fRemote)
        {
           it = fRegions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bipc::mapped_region* Manager::CreateRegion(const size_t size, const uint64_t id, FairMQRegionCallback callback)
{
    auto it = fRegions.find(id);
    if (it != fRegions.end())
    {
        LOG(error) << "Trying to create a region that already exists";
        return nullptr;
    }
    else
    {
        auto r = fRegions.emplace(id, Region{*this, id, size, false, callback});

        r.first->second.StartReceivingAcks();

        return &(r.first->second.fRegion);
    }
}

Region* Manager::GetRemoteRegion(const uint64_t id)
{
    // remote region could actually be a local one if a message originates from this device (has been sent out and returned)
    auto it = fRegions.find(id);
    if (it != fRegions.end())
    {
        return &(it->second);
    }
    else
    {
        try
        {
            auto r = fRegions.emplace(id, Region{*this, id, 0, true, nullptr});
            return &(r.first->second);
        }
        catch (bipc::interprocess_exception& e)
        {
            // LOG(warn) << "remote region (" << id << ") no longer exists";
            return nullptr;
        }

    }
}

void Manager::RemoveRegion(const uint64_t id)
{
    fRegions.erase(id);
}

void Manager::RemoveSegment()
{
    if (bipc::shared_memory_object::remove(fSegmentName.c_str()))
    {
        LOG(debug) << "successfully removed " << fSegmentName << " segment after the device has stopped.";
    }
    else
    {
        LOG(debug) << "did not remove " << fSegmentName << " segment after the device stopped. Already removed?";
    }

    if (bipc::shared_memory_object::remove(fManagementSegmentName.c_str()))
    {
        LOG(debug) << "successfully removed '" << fManagementSegmentName << "' segment after the device has stopped.";
    }
    else
    {
        LOG(debug) << "did not remove '" << fManagementSegmentName << "' segment after the device stopped. Already removed?";
    }
}

bipc::managed_shared_memory& Manager::ManagementSegment()
{
    return fManagementSegment;
}

} // namespace shmem
} // namespace mq
} // namespace fair

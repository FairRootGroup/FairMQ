/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQUnmanagedRegionSHM.h"
#include "FairMQShmManager.h"
#include "FairMQShmCommon.h"

using namespace std;
using namespace fair::mq::shmem;

namespace bipc = boost::interprocess;

atomic<bool> FairMQUnmanagedRegionSHM::fInterrupted(false);
unordered_map<uint64_t, RemoteRegion> FairMQUnmanagedRegionSHM::fRemoteRegionMap;

FairMQUnmanagedRegionSHM::FairMQUnmanagedRegionSHM(const size_t size)
    : fRegion(nullptr)
    , fRegionId(0)
    , fRegionIdStr()
    , fRemote(false)
{
    try
    {
        RegionCounter* rc = Manager::Instance().ManagementSegment().find<RegionCounter>(bipc::unique_instance).first;
        if (rc)
        {
            LOG(DEBUG) << "shmem: region counter found, with value of " << rc->fCount << ". incrementing.";
            (rc->fCount)++;
            LOG(DEBUG) << "shmem: incremented region counter, now: " << rc->fCount;
        }
        else
        {
            LOG(DEBUG) << "shmem: no region counter found, creating one and initializing with 1";
            rc = Manager::Instance().ManagementSegment().construct<RegionCounter>(bipc::unique_instance)(1);
            LOG(DEBUG) << "shmem: initialized region counter with: " << rc->fCount;
        }

        fRegionId = rc->fCount;
        fRegionIdStr = "fairmq_shmem_region_" + to_string(fRegionId);

        auto it = fRemoteRegionMap.find(fRegionId);
        if (it != fRemoteRegionMap.end())
        {
            LOG(ERROR) << "Trying to create a region that already exists";
        }
        else
        {
            string regionIdStr = "fairmq_shmem_region_" + to_string(fRegionId);

            LOG(DEBUG) << "creating region with id " << fRegionId;

            auto r = fRemoteRegionMap.emplace(fRegionId, RemoteRegion{regionIdStr, size});
            fRegion = &(r.first->second.fRegion);

            LOG(DEBUG) << "created region with id " << fRegionId;
        }
    }
    catch (bipc::interprocess_exception& e)
    {
        LOG(ERROR) << "shmem: cannot create region. Already created/not cleaned up?";
        LOG(ERROR) << e.what();
        exit(EXIT_FAILURE);
    }
}

void* FairMQUnmanagedRegionSHM::GetData() const
{
    return fRegion->get_address();
}

size_t FairMQUnmanagedRegionSHM::GetSize() const
{
    return fRegion->get_size();
}

bipc::mapped_region* FairMQUnmanagedRegionSHM::GetRemoteRegion(uint64_t regionId)
{
    auto it = fRemoteRegionMap.find(regionId);
    if (it != fRemoteRegionMap.end())
    {
        return &(it->second.fRegion);
    }
    else
    {
        string regionIdStr = "fairmq_shmem_region_" + to_string(regionId);

        auto r = fRemoteRegionMap.emplace(regionId, RemoteRegion{regionIdStr, 0});
        return &(r.first->second.fRegion);
    }
}

FairMQUnmanagedRegionSHM::~FairMQUnmanagedRegionSHM()
{
}

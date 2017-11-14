/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQUnmanagedRegionSHM.h"
#include "Common.h"

using namespace std;
using namespace fair::mq::shmem;

namespace bipc = boost::interprocess;

FairMQUnmanagedRegionSHM::FairMQUnmanagedRegionSHM(Manager& manager, const size_t size, FairMQRegionCallback callback)
    : fManager(manager)
    , fRegion(nullptr)
    , fRegionId(0)
{
    try
    {
        RegionCounter* rc = fManager.ManagementSegment().find<RegionCounter>(bipc::unique_instance).first;
        if (rc)
        {
            LOG(DEBUG) << "shmem: region counter found, with value of " << rc->fCount << ". incrementing.";
            (rc->fCount)++;
            LOG(DEBUG) << "shmem: incremented region counter, now: " << rc->fCount;
        }
        else
        {
            LOG(DEBUG) << "shmem: no region counter found, creating one and initializing with 1";
            rc = fManager.ManagementSegment().construct<RegionCounter>(bipc::unique_instance)(1);
            LOG(DEBUG) << "shmem: initialized region counter with: " << rc->fCount;
        }

        fRegionId = rc->fCount;

        fRegion = fManager.CreateRegion(size, fRegionId, callback);
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

FairMQUnmanagedRegionSHM::~FairMQUnmanagedRegionSHM()
{
    fManager.RemoveRegion(fRegionId);
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Common.h"

#include "FairMQUnmanagedRegionSHM.h"

using namespace std;
using namespace fair::mq::shmem;

namespace bipc = ::boost::interprocess;

FairMQUnmanagedRegionSHM::FairMQUnmanagedRegionSHM(Manager& manager, const size_t size, FairMQRegionCallback callback, const std::string& path /* = "" */, int flags /* = 0 */)
    : fManager(manager)
    , fRegion(nullptr)
    , fRegionId(0)
{
    try {
        RegionCounter* rc = fManager.ManagementSegment().find<RegionCounter>(bipc::unique_instance).first;
        if (rc) {
            LOG(debug) << "region counter found, with value of " << rc->fCount << ". incrementing.";
            (rc->fCount)++;
            LOG(debug) << "incremented region counter, now: " << rc->fCount;
        } else {
            LOG(debug) << "no region counter found, creating one and initializing with 1";
            rc = fManager.ManagementSegment().construct<RegionCounter>(bipc::unique_instance)(1);
            LOG(debug) << "initialized region counter with: " << rc->fCount;
        }

        fRegionId = rc->fCount;

        fRegion = fManager.CreateRegion(size, fRegionId, callback, path, flags);
    } catch (bipc::interprocess_exception& e) {
        LOG(error) << "cannot create region. Already created/not cleaned up?";
        LOG(error) << e.what();
        throw;
    }
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQRegionSHM.h"
#include "FairMQShmManager.h"
#include "FairMQShmCommon.h"

using namespace std;
using namespace fair::mq::shmem;

namespace bipc = boost::interprocess;

atomic<bool> FairMQRegionSHM::fInterrupted(false);

FairMQRegionSHM::FairMQRegionSHM(const size_t size)
    : fShmemObject()
    , fRegion()
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
        fRegionIdStr = "fairmq_shmem_region_" + std::to_string(fRegionId);

        fShmemObject = unique_ptr<bipc::shared_memory_object>(new bipc::shared_memory_object(bipc::create_only, fRegionIdStr.c_str(), bipc::read_write));
        fShmemObject->truncate(size);
        fRegion = unique_ptr<bipc::mapped_region>(new bipc::mapped_region(*fShmemObject, bipc::read_write)); // TODO: add HUGEPAGES flag here
    }
    catch (bipc::interprocess_exception& e)
    {
        LOG(ERROR) << "shmem: cannot create region. Already created/not cleaned up?";
        LOG(ERROR) << e.what();
        exit(EXIT_FAILURE);
    }
}

FairMQRegionSHM::FairMQRegionSHM(const uint64_t id, bool remote)
    : fShmemObject()
    , fRegion()
    , fRegionId(id)
    , fRegionIdStr()
    , fRemote(remote)
{
    try
    {
        fRegionIdStr = "fairmq_shmem_region_" + std::to_string(fRegionId);

        fShmemObject = unique_ptr<bipc::shared_memory_object>(new bipc::shared_memory_object(bipc::open_only, fRegionIdStr.c_str(), bipc::read_write));
        fRegion = unique_ptr<bipc::mapped_region>(new bipc::mapped_region(*fShmemObject, bipc::read_write)); // TODO: add HUGEPAGES flag here
    }
    catch (bipc::interprocess_exception& e)
    {
        LOG(ERROR) << "shmem: cannot open region. Already closed?";
        LOG(ERROR) << e.what();
        exit(EXIT_FAILURE);
    }
}

void* FairMQRegionSHM::GetData() const
{
    return fRegion->get_address();
}

size_t FairMQRegionSHM::GetSize() const
{
    return fRegion->get_size();
}

FairMQRegionSHM::~FairMQRegionSHM()
{
    if (!fRemote)
    {
        LOG(DEBUG) << "destroying region";
        bipc::shared_memory_object::remove(fRegionIdStr.c_str());
    }
}

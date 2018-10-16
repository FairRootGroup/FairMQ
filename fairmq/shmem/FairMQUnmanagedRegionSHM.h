/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQUNMANAGEDREGIONSHM_H_
#define FAIRMQUNMANAGEDREGIONSHM_H_

#include <fairmq/shmem/Manager.h>

#include "FairMQUnmanagedRegion.h"
#include "FairMQLogger.h"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <cstddef> // size_t

class FairMQUnmanagedRegionSHM final : public FairMQUnmanagedRegion
{
    friend class FairMQSocketSHM;
    friend class FairMQMessageSHM;

  public:
    FairMQUnmanagedRegionSHM(fair::mq::shmem::Manager& manager, const size_t size, FairMQRegionCallback callback = nullptr);

    void* GetData() const override;
    size_t GetSize() const override;

    ~FairMQUnmanagedRegionSHM() override;

  private:
    fair::mq::shmem::Manager& fManager;
    boost::interprocess::mapped_region* fRegion;
    uint64_t fRegionId;
};

#endif /* FAIRMQUNMANAGEDREGIONSHM_H_ */
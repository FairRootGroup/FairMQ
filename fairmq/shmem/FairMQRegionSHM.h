/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQREGIONSHM_H_
#define FAIRMQREGIONSHM_H_

#include "FairMQRegion.h"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <cstddef> // size_t
#include <atomic>
#include <memory>
#include <string>

class FairMQRegionSHM : public FairMQRegion
{
    friend class FairMQSocketSHM;
    friend class FairMQMessageSHM;

  public:
    FairMQRegionSHM(const size_t size);

    virtual void* GetData() const override;
    virtual size_t GetSize() const override;

    virtual ~FairMQRegionSHM();

  private:
    FairMQRegionSHM(const uint64_t id, bool remote);

    static std::atomic<bool> fInterrupted;
    std::unique_ptr<boost::interprocess::shared_memory_object> fShmemObject;
    std::unique_ptr<boost::interprocess::mapped_region> fRegion;
    uint64_t fRegionId;
    std::string fRegionIdStr;
    bool fRemote;
};

#endif /* FAIRMQREGIONSHM_H_ */
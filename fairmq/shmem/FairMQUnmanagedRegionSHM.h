/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQUNMANAGEDREGIONSHM_H_
#define FAIRMQUNMANAGEDREGIONSHM_H_

#include "FairMQUnmanagedRegion.h"
#include "FairMQLogger.h"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <cstddef> // size_t
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>

struct RemoteRegion // todo: better name?
{
    RemoteRegion(std::string regionIdStr, uint64_t size)
        : fRegionName(regionIdStr)
        , fShmemObject(boost::interprocess::open_or_create, regionIdStr.c_str(), boost::interprocess::read_write)
    {
        if (size > 0)
        {
            fShmemObject.truncate(size);
        }
        fRegion = boost::interprocess::mapped_region(fShmemObject, boost::interprocess::read_write); // TODO: add HUGEPAGES flag here
    }

    RemoteRegion() = delete;

    RemoteRegion(const RemoteRegion& rr) = default;
    RemoteRegion(RemoteRegion&& rr) = default;

    ~RemoteRegion()
    {
        if (boost::interprocess::shared_memory_object::remove(fRegionName.c_str()))
        {
            LOG(DEBUG) << "destroyed region " << fRegionName;
        }
    }

    std::string fRegionName;
    boost::interprocess::shared_memory_object fShmemObject;
    boost::interprocess::mapped_region fRegion;
};

class FairMQUnmanagedRegionSHM : public FairMQUnmanagedRegion
{
    friend class FairMQSocketSHM;
    friend class FairMQMessageSHM;

  public:
    FairMQUnmanagedRegionSHM(const size_t size);

    virtual void* GetData() const override;
    virtual size_t GetSize() const override;

    static boost::interprocess::mapped_region* GetRemoteRegion(uint64_t regionId);

    virtual ~FairMQUnmanagedRegionSHM();

  private:
    static std::atomic<bool> fInterrupted;
    boost::interprocess::mapped_region* fRegion;
    uint64_t fRegionId;
    std::string fRegionIdStr;
    bool fRemote;
    static std::unordered_map<uint64_t, RemoteRegion> fRemoteRegionMap;
};

#endif /* FAIRMQUNMANAGEDREGIONSHM_H_ */
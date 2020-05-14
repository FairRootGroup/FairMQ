/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SHMEM_UNMANAGEDREGION_H_
#define FAIR_MQ_SHMEM_UNMANAGEDREGION_H_

#include "Manager.h"

#include <FairMQUnmanagedRegion.h>
#include <FairMQLogger.h>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <cstddef> // size_t
#include <string>

namespace fair
{
namespace mq
{
namespace shmem
{

class Message;
class Socket;

class UnmanagedRegion final : public fair::mq::UnmanagedRegion
{
    friend class Message;
    friend class Socket;

  public:
    UnmanagedRegion(Manager& manager,
                    const size_t size,
                    const int64_t userFlags,
                    RegionCallback callback,
                    RegionBulkCallback bulkCallback,
                    const std::string& path = "",
                    int flags = 0,
                    FairMQTransportFactory* factory = nullptr)
        : FairMQUnmanagedRegion(factory)
        , fManager(manager)
        , fRegion(nullptr)
        , fRegionId(0)
    {
        auto result = fManager.CreateRegion(size, userFlags, callback, bulkCallback, path, flags);
        fRegion = result.first;
        fRegionId = result.second;
    }

    void* GetData() const override { return fRegion->get_address(); }
    size_t GetSize() const override { return fRegion->get_size(); }
    uint64_t GetId() const override { return fRegionId; }

    ~UnmanagedRegion() override { fManager.RemoveRegion(fRegionId); }

  private:
    Manager& fManager;
    boost::interprocess::mapped_region* fRegion;
    uint64_t fRegionId;
};

}
}
}

#endif /* FAIR_MQ_SHMEM_UNMANAGEDREGION_H_ */

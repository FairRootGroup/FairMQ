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

namespace fair::mq::shmem
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
                    const std::string& path,
                    int flags,
                    FairMQTransportFactory* factory,
                    fair::mq::RegionConfig cfg)
        : FairMQUnmanagedRegion(factory)
        , fManager(manager)
        , fRegion(nullptr)
        , fRegionId(0)
    {
        auto result = fManager.CreateRegion(size, userFlags, callback, bulkCallback, path, flags, cfg);
        fRegion = result.first;
        fRegionId = result.second;
    }

    UnmanagedRegion(const UnmanagedRegion&) = delete;
    UnmanagedRegion(UnmanagedRegion&&) = delete;
    UnmanagedRegion& operator=(const UnmanagedRegion&) = delete;
    UnmanagedRegion& operator=(UnmanagedRegion&&) = delete;

    void* GetData() const override { return fRegion->get_address(); }
    size_t GetSize() const override { return fRegion->get_size(); }
    uint16_t GetId() const override { return fRegionId; }
    void SetLinger(uint32_t linger) override { fManager.GetRegion(fRegionId)->SetLinger(linger); }
    uint32_t GetLinger() const override { return fManager.GetRegion(fRegionId)->GetLinger(); }

    Transport GetType() const override { return fair::mq::Transport::SHM; }

    ~UnmanagedRegion() override { fManager.RemoveRegion(fRegionId); }

  private:
    Manager& fManager;
    boost::interprocess::mapped_region* fRegion;
    uint16_t fRegionId;
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_UNMANAGEDREGION_H_ */

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SHMEM_UNMANAGEDREGIONIMPL_H_
#define FAIR_MQ_SHMEM_UNMANAGEDREGIONIMPL_H_

#include "Manager.h"
#include "UnmanagedRegion.h"
#include <fairmq/UnmanagedRegion.h>

#include <fairlogger/Logger.h>

#include <cstddef> // size_t

namespace fair::mq::shmem
{

class Message;
class Socket;

class UnmanagedRegionImpl final : public fair::mq::UnmanagedRegion
{
    friend class Message;
    friend class Socket;

  public:
    UnmanagedRegionImpl(Manager& manager,
                    const size_t size,
                    RegionCallback callback,
                    RegionBulkCallback bulkCallback,
                    fair::mq::RegionConfig cfg,
                    fair::mq::TransportFactory* factory)
        : fair::mq::UnmanagedRegion(factory)
        , fManager(manager)
        , fRegion(nullptr)
        , fRegionId(0)
    {
        auto [regionPtr, regionId] = fManager.CreateRegion(size, callback, bulkCallback, std::move(cfg));
        fRegion = regionPtr;
        fRegionId = regionId;
    }

    UnmanagedRegionImpl(const UnmanagedRegionImpl&) = delete;
    UnmanagedRegionImpl(UnmanagedRegionImpl&&) = delete;
    UnmanagedRegionImpl& operator=(const UnmanagedRegionImpl&) = delete;
    UnmanagedRegionImpl& operator=(UnmanagedRegionImpl&&) = delete;

    void* GetData() const override { return fRegion->GetData(); }
    size_t GetSize() const override { return fRegion->GetSize(); }
    uint16_t GetId() const override { return fRegionId; }
    void SetLinger(uint32_t linger) override { fRegion->SetLinger(linger); }
    uint32_t GetLinger() const override { return fRegion->GetLinger(); }

    Transport GetType() const override { return fair::mq::Transport::SHM; }

    ~UnmanagedRegionImpl() override { fManager.RemoveRegion(fRegionId); }

  private:
    Manager& fManager;
    shmem::UnmanagedRegion* fRegion;
    uint16_t fRegionId;
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_UNMANAGEDREGIONIMPL_H_ */

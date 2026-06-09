/********************************************************************************
 * Copyright (C) 2016-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SHMEM_TRANSPORTFACTORY_H_
#define FAIR_MQ_SHMEM_TRANSPORTFACTORY_H_

#include "Common.h"
#include "Manager.h"
#include "Message.h"

#include <fairmq/TransportFactory.h>

#include <boost/version.hpp>

#include <string>
#include <vector>

namespace fair::mq::shmem
{

class TransportFactory final : public fair::mq::TransportFactory
{
  public:
    TransportFactory(const std::string& deviceId = "", const ProgOptions* config = nullptr);

    TransportFactory(const TransportFactory&) = delete;
    TransportFactory(TransportFactory&&) = delete;
    TransportFactory& operator=(const TransportFactory&) = delete;
    TransportFactory& operator=(TransportFactory&&) = delete;

    inline MessagePtr CreateMessage() override;
    inline MessagePtr CreateMessage(Alignment alignment) override;
    inline MessagePtr CreateMessage(size_t size) override;
    inline MessagePtr CreateMessage(size_t size, Alignment alignment) override;
    inline MessagePtr CreateMessage(void* data, size_t size, fair::mq::FreeFn* ffn, void* hint = nullptr) override;
    inline MessagePtr CreateMessage(UnmanagedRegionPtr& region, void* data, size_t size, void* hint = nullptr) override;
    inline SocketPtr CreateSocket(const std::string& type, const std::string& name) override;
    inline PollerPtr CreatePoller(const std::vector<Channel>& channels) const override;
    inline PollerPtr CreatePoller(const std::vector<Channel*>& channels) const override;
    inline PollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<Channel>>& channelsMap, const std::vector<std::string>& channelList) const override;
    inline UnmanagedRegionPtr CreateUnmanagedRegion(size_t size, RegionCallback callback = nullptr, const std::string& path = "", int flags = 0, fair::mq::RegionConfig cfg = fair::mq::RegionConfig()) override;
    inline UnmanagedRegionPtr CreateUnmanagedRegion(size_t size, RegionBulkCallback bulkCallback = nullptr, const std::string& path = "", int flags = 0, fair::mq::RegionConfig cfg = fair::mq::RegionConfig()) override;
    inline UnmanagedRegionPtr CreateUnmanagedRegion(size_t size, int64_t userFlags, RegionCallback callback = nullptr, const std::string& path = "", int flags = 0, fair::mq::RegionConfig cfg = fair::mq::RegionConfig()) override;
    inline UnmanagedRegionPtr CreateUnmanagedRegion(size_t size, int64_t userFlags, RegionBulkCallback bulkCallback = nullptr, const std::string& path = "", int flags = 0, fair::mq::RegionConfig cfg = fair::mq::RegionConfig()) override;
    inline UnmanagedRegionPtr CreateUnmanagedRegion(size_t size, RegionCallback callback, RegionConfig cfg) override;
    inline UnmanagedRegionPtr CreateUnmanagedRegion(size_t size, RegionBulkCallback bulkCallback, RegionConfig cfg) override;
    inline UnmanagedRegionPtr CreateUnmanagedRegion(size_t size, RegionCallback callback, RegionBulkCallback bulkCallback, fair::mq::RegionConfig cfg);
    inline void SubscribeToRegionEvents(RegionEventCallback callback) override;
    inline bool SubscribedToRegionEvents() override;
    inline void UnsubscribeFromRegionEvents() override;
    inline std::vector<fair::mq::RegionInfo> GetRegionInfo() override;

    Transport GetType() const override { return fair::mq::Transport::SHM; }

    void Interrupt() override { fManager->Interrupt(); }
    void Resume() override { fManager->Resume(); }
    void Reset() override { fManager->Reset(); }

    ~TransportFactory() override;

    Manager* GetManager()
    {
      return fManager.get();
    }

  private:
    void* fZmqCtx;
    std::unique_ptr<Manager> fManager;
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_TRANSPORTFACTORY_H_ */

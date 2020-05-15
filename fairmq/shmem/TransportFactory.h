/********************************************************************************
 * Copyright (C) 2016-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SHMEM_TRANSPORTFACTORY_H_
#define FAIR_MQ_SHMEM_TRANSPORTFACTORY_H_

#include "Manager.h"
#include "Common.h"
#include "Message.h"
#include "Socket.h"
#include "Poller.h"
#include "UnmanagedRegion.h"

#include <FairMQTransportFactory.h>
#include <fairmq/ProgOptions.h>

#include <vector>
#include <string>
#include <thread>
#include <atomic>

namespace fair
{
namespace mq
{
namespace shmem
{

class TransportFactory final : public fair::mq::TransportFactory
{
  public:
    TransportFactory(const std::string& id = "", const ProgOptions* config = nullptr);
    TransportFactory(const TransportFactory&) = delete;
    TransportFactory operator=(const TransportFactory&) = delete;

    MessagePtr CreateMessage() override;
    MessagePtr CreateMessage(const size_t size) override;
    MessagePtr CreateMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override;
    MessagePtr CreateMessage(UnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0) override;

    SocketPtr CreateSocket(const std::string& type, const std::string& name) override;

    PollerPtr CreatePoller(const std::vector<FairMQChannel>& channels) const override;
    PollerPtr CreatePoller(const std::vector<FairMQChannel*>& channels) const override;
    PollerPtr CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const override;

    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, RegionCallback callback = nullptr, const std::string& path = "", int flags = 0) override;
    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, RegionBulkCallback callback = nullptr, const std::string& path = "", int flags = 0) override;
    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, int64_t userFlags, RegionCallback callback = nullptr, const std::string& path = "", int flags = 0) override;
    UnmanagedRegionPtr CreateUnmanagedRegion(const size_t size, int64_t userFlags, RegionBulkCallback callback = nullptr, const std::string& path = "", int flags = 0) override;

    void SubscribeToRegionEvents(RegionEventCallback callback) override;
    bool SubscribedToRegionEvents() override;
    void UnsubscribeFromRegionEvents() override;
    std::vector<fair::mq::RegionInfo> GetRegionInfo() override;

    Transport GetType() const override { return fair::mq::Transport::SHM; }

    void Interrupt() override { fManager->Interrupt(); }
    void Resume() override { fManager->Resume(); }
    void Reset() override;

    void IncrementMsgCounter() { ++fMsgCounter; }
    void DecrementMsgCounter() { --fMsgCounter; }

    ~TransportFactory() override;

  private:
    void SendHeartbeats();

    std::string fDeviceId;
    std::string fShmId;
    void* fZMQContext;
    std::unique_ptr<Manager> fManager;
    std::thread fHeartbeatThread;
    std::atomic<bool> fSendHeartbeats;
    std::atomic<int32_t> fMsgCounter;
};

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_TRANSPORTFACTORY_H_ */

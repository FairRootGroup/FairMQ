/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_OFI_TRANSPORTFACTORY_H
#define FAIR_MQ_OFI_TRANSPORTFACTORY_H

#include <FairMQTransportFactory.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/ofi/Context.h>

#include <asiofi.hpp>

namespace fair
{
namespace mq
{
namespace ofi
{

/**
 * @class TransportFactory TransportFactory.h <fairmq/ofi/TransportFactory.h>
 * @brief FairMQ transport factory for the ofi transport
 *
 * @todo TODO insert long description
 */
class TransportFactory final : public FairMQTransportFactory
{
  public:
    TransportFactory(const std::string& id = "", const fair::mq::ProgOptions* config = nullptr);
    TransportFactory(const TransportFactory&) = delete;
    TransportFactory operator=(const TransportFactory&) = delete;

    auto CreateMessage() -> MessagePtr override;
    auto CreateMessage(const std::size_t size) -> MessagePtr override;
    auto CreateMessage(void* data, const std::size_t size, fairmq_free_fn* ffn, void* hint = nullptr) -> MessagePtr override;
    auto CreateMessage(UnmanagedRegionPtr& region, void* data, const std::size_t size, void* hint = nullptr) -> MessagePtr override;

    auto CreateSocket(const std::string& type, const std::string& name) -> SocketPtr override;

    auto CreatePoller(const std::vector<FairMQChannel>& channels) const -> PollerPtr override;
    auto CreatePoller(const std::vector<FairMQChannel*>& channels) const -> PollerPtr override;
    auto CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const -> PollerPtr override;

    auto CreateUnmanagedRegion(const size_t size, RegionCallback callback = nullptr, const std::string& path = "", int flags = 0) const -> UnmanagedRegionPtr override;
    auto CreateUnmanagedRegion(const size_t size, int64_t userFlags, RegionCallback callback = nullptr, const std::string& path = "", int flags = 0) const -> UnmanagedRegionPtr override;

    void SubscribeToRegionEvents(RegionEventCallback /* callback */) override { LOG(error) << "SubscribeToRegionEvents not yet implemented for OFI"; }
    void UnsubscribeFromRegionEvents() override { LOG(error) << "UnsubscribeFromRegionEvents not yet implemented for OFI"; }
    std::vector<FairMQRegionInfo> GetRegionInfo() override { LOG(error) << "GetRegionInfo not yet implemented for OFI, returning empty vector"; return std::vector<FairMQRegionInfo>(); }

    auto GetType() const -> Transport override;

    void Interrupt() override { fContext.Interrupt(); }
    void Resume() override { fContext.Resume(); }
    void Reset() override { fContext.Reset(); }

  private:
    mutable Context fContext;
    asiofi::allocated_pool_resource fMemoryResource;
}; /* class TransportFactory */  

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_OFI_TRANSPORTFACTORY_H */

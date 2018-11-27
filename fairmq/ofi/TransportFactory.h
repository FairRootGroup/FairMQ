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
#include <options/FairMQProgOptions.h>
#include <fairmq/ofi/Context.h>

namespace fair
{
namespace mq
{
namespace ofi
{

/**
 * @class TransportFactory TransportFactory.h <fairmq/ofi/TransportFactory.h>
 * @brief FairMQ transport factory for the ofi transport (implemented with ZeroMQ + libfabric)
 *
 * @todo TODO insert long description
 */
class TransportFactory final : public FairMQTransportFactory
{
  public:
    TransportFactory(const std::string& id = "", const FairMQProgOptions* config = nullptr);
    TransportFactory(const TransportFactory&) = delete;
    TransportFactory operator=(const TransportFactory&) = delete;

    auto CreateMessage() const -> MessagePtr override;
    auto CreateMessage(const std::size_t size) const -> MessagePtr override;
    auto CreateMessage(void* data, const std::size_t size, fairmq_free_fn* ffn, void* hint = nullptr) const -> MessagePtr override;
    auto CreateMessage(UnmanagedRegionPtr& region, void* data, const std::size_t size, void* hint = nullptr) const -> MessagePtr override;

    auto CreateSocket(const std::string& type, const std::string& name) -> SocketPtr override;

    auto CreatePoller(const std::vector<FairMQChannel>& channels) const -> PollerPtr override;
    auto CreatePoller(const std::vector<const FairMQChannel*>& channels) const -> PollerPtr override;
    auto CreatePoller(const std::unordered_map<std::string, std::vector<FairMQChannel>>& channelsMap, const std::vector<std::string>& channelList) const -> PollerPtr override;

    auto CreateUnmanagedRegion(const size_t size, FairMQRegionCallback callback = nullptr) const -> UnmanagedRegionPtr override;

    auto GetType() const -> Transport override;

    void Interrupt() override {}
    void Resume() override {}
    void Reset() override {}

  private:
    mutable Context fContext;
}; /* class TransportFactory */  

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_OFI_TRANSPORTFACTORY_H */

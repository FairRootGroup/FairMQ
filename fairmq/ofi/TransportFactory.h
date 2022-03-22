/********************************************************************************
 * Copyright (C) 2018-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_OFI_TRANSPORTFACTORY_H
#define FAIR_MQ_OFI_TRANSPORTFACTORY_H

#include <asiofi.hpp>
#include <cstddef>
#include <cstdint>
#include <fairlogger/Logger.h>
#include <fairmq/Channel.h>
#include <fairmq/Message.h>
#include <fairmq/Poller.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/Socket.h>
#include <fairmq/TransportFactory.h>
#include <fairmq/Transports.h>
#include <fairmq/ofi/Context.h>
#include <fairmq/ofi/Message.h>
#include <fairmq/ofi/Socket.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace fair::mq::ofi {

/**
 * @class TransportFactory TransportFactory.h <fairmq/ofi/TransportFactory.h>
 * @brief FairMQ transport factory for the ofi transport
 *
 * @todo TODO insert long description
 */
struct TransportFactory final : mq::TransportFactory
{
    TransportFactory(std::string const& id = "", ProgOptions const* config = nullptr)
        : mq::TransportFactory(id)
        , fContext(*this, *this, 1)
    {
        try {
            LOG(debug) << "OFI transport: asiofi (" << fContext.GetAsiofiVersion() << ")";

            if (config) {
                fContext.SetSizeHint(config->GetProperty<size_t>("ofi-size-hint", 0));
            }
        } catch (ContextError& e) {
            throw TransportFactoryError(e.what());
        }
    }

    TransportFactory(const TransportFactory&) = delete;
    TransportFactory(TransportFactory&&) = delete;
    TransportFactory& operator=(const TransportFactory&) = delete;
    TransportFactory& operator=(TransportFactory&&) = delete;
    ~TransportFactory() override = default;

    auto CreateMessage() -> std::unique_ptr<mq::Message> override
    {
        return std::make_unique<Message>(&fMemoryResource);
    }

    auto CreateMessage(Alignment /*alignment*/) -> std::unique_ptr<mq::Message> override
    {
        // TODO Do not ignore alignment
        return std::make_unique<Message>(&fMemoryResource);
    }

    auto CreateMessage(std::size_t size) -> std::unique_ptr<mq::Message> override
    {
        return std::make_unique<Message>(&fMemoryResource, size);
    }

    auto CreateMessage(std::size_t size, Alignment /*alignment*/)
        -> std::unique_ptr<mq::Message> override
    {
        // TODO Do not ignore alignment
        return std::make_unique<Message>(&fMemoryResource, size);
    }

    auto CreateMessage(void* data, std::size_t size, FreeFn* ffn, void* hint = nullptr)
        -> std::unique_ptr<mq::Message> override
    {
        return std::make_unique<Message>(&fMemoryResource, data, size, ffn, hint);
    }

    auto CreateMessage(std::unique_ptr<mq::UnmanagedRegion>& region,
                       void* data,
                       std::size_t size,
                       void* hint = nullptr) -> std::unique_ptr<mq::Message> override
    {
        return std::make_unique<Message>(&fMemoryResource, region, data, size, hint);
    }

    auto CreateSocket(std::string const& type, std::string const& name)
        -> std::unique_ptr<mq::Socket> override
    {
        return std::make_unique<Socket>(fContext, type, name, GetId());
    }

    auto CreatePoller(std::vector<mq::Channel> const& /*channels*/) const
        -> std::unique_ptr<mq::Poller> override
    {
        throw std::runtime_error("Not yet implemented (Poller).");
    }

    auto CreatePoller(std::vector<mq::Channel*> const& /*channels*/) const
        -> std::unique_ptr<mq::Poller> override
    {
        throw std::runtime_error("Not yet implemented (Poller).");
    }

    auto CreatePoller(
        std::unordered_map<std::string, std::vector<Channel>> const& /*channelsMap*/,
        std::vector<std::string> const& /*channelList*/) const
        -> std::unique_ptr<mq::Poller> override
    {
        throw std::runtime_error("Not yet implemented (Poller).");
    }

    auto CreateUnmanagedRegion(std::size_t /*size*/,
                               RegionCallback /*callback = nullptr*/,
                               std::string const& /*path = ""*/,
                               int /*flags = 0*/,
                               RegionConfig /*cfg = RegionConfig()*/)
        -> std::unique_ptr<mq::UnmanagedRegion> override
    {
        throw std::runtime_error("Not yet implemented UMR.");
    }

    auto CreateUnmanagedRegion(std::size_t /*size*/,
                               RegionBulkCallback /*callback = nullptr*/,
                               std::string const& /*path = ""*/,
                               int /*flags = 0*/,
                               RegionConfig /*cfg = RegionConfig()*/)
        -> std::unique_ptr<mq::UnmanagedRegion> override
    {
        throw std::runtime_error("Not yet implemented UMR.");
    }

    auto CreateUnmanagedRegion(std::size_t /*size*/,
                               int64_t /*userFlags*/,
                               RegionCallback /*callback = nullptr*/,
                               std::string const& /*path = ""*/,
                               int /*flags = 0*/,
                               RegionConfig /*cfg = RegionConfig()*/)
        -> std::unique_ptr<mq::UnmanagedRegion> override
    {
        throw std::runtime_error("Not yet implemented UMR.");
    }

    auto CreateUnmanagedRegion(std::size_t /*size*/,
                               int64_t /*userFlags*/,
                               RegionBulkCallback /*callback = nullptr*/,
                               std::string const& /*path = ""*/,
                               int /*flags = 0*/,
                               RegionConfig /*cfg = RegionConfig()*/)
        -> std::unique_ptr<mq::UnmanagedRegion> override
    {
        throw std::runtime_error("Not yet implemented UMR.");
    }

    auto CreateUnmanagedRegion(std::size_t /*size*/,
                               RegionCallback /*callback*/,
                               RegionConfig /*cfg*/)
        -> std::unique_ptr<mq::UnmanagedRegion> override
    {
        throw std::runtime_error("Not yet implemented UMR.");
    }

    auto CreateUnmanagedRegion(std::size_t /*size*/,
                               RegionBulkCallback /*callback*/,
                               RegionConfig /*cfg*/)
        -> std::unique_ptr<mq::UnmanagedRegion> override
    {
        throw std::runtime_error("Not yet implemented UMR.");
    }

    auto SubscribeToRegionEvents(RegionEventCallback /*callback*/) -> void override
    {
        throw std::runtime_error("Not yet implemented.");
    }

    auto SubscribedToRegionEvents() -> bool override
    {
        throw std::runtime_error("Not yet implemented.");
    }

    auto UnsubscribeFromRegionEvents() -> void override
    {
        throw std::runtime_error("Not yet implemented.");
    }

    auto GetRegionInfo() -> std::vector<RegionInfo> override
    {
        LOG(error) << "GetRegionInfo not yet implemented for OFI, returning empty vector";
        return std::vector<RegionInfo>();
    }

    auto GetType() const -> Transport override { return Transport::OFI; }

    void Interrupt() override { fContext.Interrupt(); }
    void Resume() override { fContext.Resume(); }
    void Reset() override { fContext.Reset(); }

  private:
    mutable Context fContext;
    asiofi::allocated_pool_resource fMemoryResource;
}; /* class TransportFactory */

}   // namespace fair::mq::ofi

#endif /* FAIR_MQ_OFI_TRANSPORTFACTORY_H */

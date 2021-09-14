/********************************************************************************
 * Copyright (C) 2018-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_OFI_MESSAGE_H
#define FAIR_MQ_OFI_MESSAGE_H

#include <asiofi.hpp>
#include <atomic>
#include <cstddef> // size_t
#include <fairmq/Message.h>
#include <fairmq/Transports.h>
#include <fairmq/UnmanagedRegion.h>
#include <memory_resource>
#include <zmq.h>

namespace fair::mq::ofi
{

/**
 * @class Message Message.h <fairmq/ofi/Message.h>
 * @brief
 *
 * @todo TODO insert long description
 */
class Message final : public fair::mq::Message
{
  public:
    Message(std::pmr::memory_resource* pmr);
    Message(std::pmr::memory_resource* pmr, Alignment alignment);
    Message(std::pmr::memory_resource* pmr, size_t size);
    Message(std::pmr::memory_resource* pmr, size_t size, Alignment alignment);
    Message(std::pmr::memory_resource* pmr,
            void* data,
            size_t size,
            fairmq_free_fn* ffn,
            void* hint = nullptr);
    Message(std::pmr::memory_resource* pmr,
            fair::mq::UnmanagedRegionPtr& region,
            void* data,
            size_t size,
            void* hint = 0);

    Message(const Message&) = delete;
    Message(Message&&) = delete;
    Message& operator=(const Message&) = delete;
    Message& operator=(Message&&) = delete;

    auto Rebuild() -> void override;
    auto Rebuild(Alignment alignment) -> void override;
    auto Rebuild(size_t size) -> void override;
    auto Rebuild(size_t size, Alignment alignment) -> void override;
    auto Rebuild(void* data, size_t size, fairmq_free_fn* ffn, void* hint = nullptr) -> void override;

    auto GetData() const -> void* override;
    auto GetSize() const -> size_t override;

    auto SetUsedSize(size_t size) -> bool override;

    auto GetType() const -> fair::mq::Transport override { return fair::mq::Transport::OFI; }

    auto Copy(const fair::mq::Message& msg) -> void override;

    ~Message() override;

  private:
    size_t fInitialSize;
    size_t fSize;
    void* fData;
    fairmq_free_fn* fFreeFunction;
    void* fHint;
    std::pmr::memory_resource* fPmr;
}; /* class Message */

} // namespace fair::mq::ofi

#endif /* FAIR_MQ_OFI_MESSAGE_H */

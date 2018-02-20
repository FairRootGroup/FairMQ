/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_OFI_MESSAGE_H
#define FAIR_MQ_OFI_MESSAGE_H

#include <FairMQMessage.h>
#include <FairMQUnmanagedRegion.h>

#include <zmq.h>

#include <cstddef> // size_t
#include <atomic>

namespace fair
{
namespace mq
{
namespace ofi
{

/**
 * @class Message Message.h <fairmq/ofi/Message.h>
 * @brief 
 *
 * @todo TODO insert long description
 */
class Message : public fair::mq::Message
{
  public:
    Message();
    Message(const size_t size);
    Message(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr);
    Message(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0);

    Message(const Message&) = delete;
    Message operator=(const Message&) = delete;

    auto Rebuild() -> void override;
    auto Rebuild(const size_t size) -> void override;
    auto Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) -> void override;

    auto GetData() const -> void* override;
    auto GetSize() const -> size_t override;

    auto SetUsedSize(const size_t size) -> bool override;

    auto GetType() const -> fair::mq::Transport override { return fair::mq::Transport::OFI; }

    auto Copy(const fair::mq::Message& msg) -> void override;
    auto Copy(const fair::mq::MessagePtr& msg) -> void override;

    ~Message() noexcept(false) override;

  private:
    size_t fSize;
}; /* class Message */  

} /* namespace ofi */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_OFI_MESSAGE_H */

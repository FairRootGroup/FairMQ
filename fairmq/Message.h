/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_MESSAGE_H
#define FAIR_MQ_MESSAGE_H

#include <cstddef>   // for size_t
#include <fairmq/Transports.h>
#include <memory>   // unique_ptr
#include <stdexcept>

namespace fair::mq {

using FreeFn = void(void* data, void* hint);
class TransportFactory;

struct Alignment
{
    size_t alignment;
    explicit operator size_t() const { return alignment; }
};

struct Message
{
    Message() = default;
    Message(TransportFactory* factory)
        : fTransport(factory)
    {}

    virtual void Rebuild() = 0;
    virtual void Rebuild(Alignment alignment) = 0;
    virtual void Rebuild(const size_t size) = 0;
    virtual void Rebuild(const size_t size, Alignment alignment) = 0;
    virtual void Rebuild(void* data, const size_t size, FreeFn* ffn, void* hint = nullptr) = 0;

    virtual void* GetData() const = 0;
    virtual size_t GetSize() const = 0;

    virtual bool SetUsedSize(const size_t size) = 0;

    virtual Transport GetType() const = 0;
    TransportFactory* GetTransport() { return fTransport; }
    void SetTransport(TransportFactory* transport) { fTransport = transport; }

    virtual void Copy(const Message& msg) = 0;

    virtual ~Message() = default;

  private:
    TransportFactory* fTransport{nullptr};
};

using MessagePtr = std::unique_ptr<Message>;

struct MessageError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

struct MessageBadAlloc : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

}   // namespace fair::mq

using fairmq_free_fn [[deprecated("Use fair::mq::FreeFn")]] = fair::mq::FreeFn;
using FairMQMessage [[deprecated("Use fair::mq::Message")]] = fair::mq::Message;
using FairMQMessagePtr [[deprecated("Use fair::mq::MessagePtr")]] = fair::mq::MessagePtr;

#endif   // FAIR_MQ_MESSAGE_H

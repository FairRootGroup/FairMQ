/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQMESSAGE_H_
#define FAIRMQMESSAGE_H_

#include <cstddef> // for size_t
#include <memory> // unique_ptr

#include <fairmq/Transports.h>

using fairmq_free_fn = void(void* data, void* hint);
class FairMQTransportFactory;

namespace fair
{
namespace mq
{

struct Alignment
{
    size_t alignment;
    explicit operator size_t() const { return alignment; }
};

} /* namespace mq */
} /* namespace fair */

class FairMQMessage
{
  public:
    FairMQMessage() = default;
    FairMQMessage(FairMQTransportFactory* factory) : fTransport(factory) {}

    virtual void Rebuild() = 0;
    virtual void Rebuild(const size_t size) = 0;
    virtual void Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) = 0;

    virtual void* GetData() const = 0;
    virtual size_t GetSize() const = 0;

    virtual bool SetUsedSize(const size_t size) = 0;

    virtual fair::mq::Transport GetType() const = 0;
    FairMQTransportFactory* GetTransport() { return fTransport; }
    void SetTransport(FairMQTransportFactory* transport) { fTransport = transport; }

    virtual void Copy(const FairMQMessage& msg) = 0;

    virtual ~FairMQMessage() {};

  private:
    FairMQTransportFactory* fTransport{nullptr};
};

using FairMQMessagePtr = std::unique_ptr<FairMQMessage>;

namespace fair
{
namespace mq
{

using Message = FairMQMessage;
using MessagePtr = FairMQMessagePtr;
struct MessageError : std::runtime_error { using std::runtime_error::runtime_error; };
struct MessageBadAlloc : std::runtime_error { using std::runtime_error::runtime_error; };

} /* namespace mq */
} /* namespace fair */

#endif /* FAIRMQMESSAGE_H_ */

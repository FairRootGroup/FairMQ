/********************************************************************************
 * Copyright (C) 2014-2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_BUFFER_H_
#define FAIR_MQ_BUFFER_H_

#include "FairMQMessage.h"

namespace fair
{
namespace mq
{

class Buffer
{
  public:
    Buffer() = delete;
    Buffer(FairMQMessagePtr&& msgPtr)
      : fBuffer(std::move(msgPtr))
    {}

    Buffer(const Buffer&) = delete;
    Buffer(Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&) = delete;

    Buffer(Buffer&&) = default;
    Buffer& operator=(Buffer&&) = default;

    void Rebuild() { fBuffer->Rebuild(); }
    void Rebuild(size_t size) { fBuffer->Rebuild(size); }
    void Rebuild(void* data, size_t size, fairmq_free_fn* ffn, void* hint = nullptr) { fBuffer->Rebuild(data, size, ffn, hint); }

    void* GetData() const { return fBuffer->GetData(); }
    size_t GetSize() const { return fBuffer->GetSize(); }

    bool SetUsedSize(size_t size) { return fBuffer->SetUsedSize(size); }

    fair::mq::Transport GetType() const { return fBuffer->GetType(); }
    FairMQTransportFactory* GetTransport() { return fBuffer->GetTransport(); }
    void SetTransport(FairMQTransportFactory* transport) { fBuffer->SetTransport(transport); }

    void Copy(const Buffer& buf) { fBuffer->Copy(*(buf.fBuffer)); }

    ~Buffer() {};

    FairMQMessagePtr fBuffer;
};

} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_BUFFER_H_ */

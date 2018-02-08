/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQMessage.h
 *
 * @since 2012-12-05
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQMESSAGE_H_
#define FAIRMQMESSAGE_H_

#include <cstddef> // for size_t
#include <memory> // unique_ptr

#include "FairMQTransports.h"

using fairmq_free_fn = void(void* data, void* hint);

class FairMQMessage
{
  public:
    virtual void Rebuild() = 0;
    virtual void Rebuild(const size_t size) = 0;
    virtual void Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) = 0;

    virtual void* GetData() const = 0;
    virtual size_t GetSize() const = 0;

    virtual bool SetUsedSize(const size_t size) = 0;

    virtual FairMQ::Transport GetType() const = 0;

    virtual void Copy(const std::unique_ptr<FairMQMessage>& msg) __attribute__((deprecated("Use 'Copy(const FairMQMessage& msg)'"))) = 0;
    virtual void Copy(const FairMQMessage& msg) = 0;

    virtual ~FairMQMessage() {};
};

using FairMQMessagePtr = std::unique_ptr<FairMQMessage>;

namespace fair
{
namespace mq
{

using MessagePtr = std::unique_ptr<FairMQMessage>;

} /* namespace mq */
} /* namespace fair */

#endif /* FAIRMQMESSAGE_H_ */

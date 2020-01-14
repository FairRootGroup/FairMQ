/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_SHMEM_MESSAGE_H_
#define FAIR_MQ_SHMEM_MESSAGE_H_

#include "Common.h"
#include "Manager.h"

#include <FairMQMessage.h>
#include <FairMQUnmanagedRegion.h>

#include <boost/interprocess/mapped_region.hpp>

#include <cstddef> // size_t
#include <atomic>

namespace fair
{
namespace mq
{
namespace shmem
{

class Socket;

class Message final : public fair::mq::Message
{
    friend class Socket;

  public:
    Message(Manager& manager, FairMQTransportFactory* factory = nullptr);
    Message(Manager& manager, const size_t size, FairMQTransportFactory* factory = nullptr);
    Message(Manager& manager, void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr, FairMQTransportFactory* factory = nullptr);
    Message(Manager& manager, UnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0, FairMQTransportFactory* factory = nullptr);

    Message(Manager& manager, MetaHeader& hdr, FairMQTransportFactory* factory = nullptr);

    Message(const Message&) = delete;
    Message operator=(const Message&) = delete;

    void Rebuild() override;
    void Rebuild(const size_t size) override;
    void Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override;

    void* GetData() const override;
    size_t GetSize() const override { return fMeta.fSize; }

    bool SetUsedSize(const size_t size) override;

    Transport GetType() const override { return fTransportType; }

    void Copy(const fair::mq::Message& msg) override;

    ~Message() override;

  private:
    Manager& fManager;
    bool fQueued;
    MetaHeader fMeta;
    mutable Region* fRegionPtr;
    mutable char* fLocalPtr;

    static std::atomic<bool> fInterrupted;
    static Transport fTransportType;

    bool InitializeChunk(const size_t size);
    void CloseMessage();
};

}
}
}

#endif /* FAIR_MQ_SHMEM_MESSAGE_H_ */

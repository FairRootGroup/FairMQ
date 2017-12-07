/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQMESSAGESHM_H_
#define FAIRMQMESSAGESHM_H_

#include <fairmq/shmem/Manager.h>

#include "FairMQMessage.h"
#include "FairMQUnmanagedRegion.h"

#include <zmq.h>

#include <boost/interprocess/mapped_region.hpp>

#include <cstddef> // size_t
#include <atomic>

class FairMQSocketSHM;

class FairMQMessageSHM : public FairMQMessage
{
    friend class FairMQSocketSHM;

  public:
    FairMQMessageSHM(fair::mq::shmem::Manager& manager);
    FairMQMessageSHM(fair::mq::shmem::Manager& manager, const size_t size);
    FairMQMessageSHM(fair::mq::shmem::Manager& manager, void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr);
    FairMQMessageSHM(fair::mq::shmem::Manager& manager, FairMQUnmanagedRegionPtr& region, void* data, const size_t size);

    FairMQMessageSHM(const FairMQMessageSHM&) = delete;
    FairMQMessageSHM operator=(const FairMQMessageSHM&) = delete;

    void Rebuild() override;
    void Rebuild(const size_t size) override;
    void Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override;

    void* GetData() override;
    size_t GetSize() const override;

    bool SetUsedSize(const size_t size) override;

    FairMQ::Transport GetType() const override;

    void Copy(const FairMQMessagePtr& msg) override;

    ~FairMQMessageSHM() override;

  private:
    fair::mq::shmem::Manager& fManager;
    zmq_msg_t fMessage;
    bool fQueued;
    bool fMetaCreated;
    static std::atomic<bool> fInterrupted;
    static FairMQ::Transport fTransportType;
    uint64_t fRegionId;
    fair::mq::shmem::Region* fRegionPtr;
    boost::interprocess::managed_shared_memory::handle_t fHandle;
    size_t fSize;
    char* fLocalPtr;

    bool InitializeChunk(const size_t size);
    zmq_msg_t* GetMessage();
    void CloseMessage();
};

#endif /* FAIRMQMESSAGESHM_H_ */

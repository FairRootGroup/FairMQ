/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQMESSAGESHM_H_
#define FAIRMQMESSAGESHM_H_

#include <cstddef> // size_t
#include <string>
#include <atomic>

#include <zmq.h>

#include <boost/interprocess/mapped_region.hpp>

#include "FairMQMessage.h"
#include "FairMQUnmanagedRegion.h"
#include "FairMQShmManager.h"

class FairMQMessageSHM : public FairMQMessage
{
    friend class FairMQSocketSHM;

  public:
    FairMQMessageSHM();
    FairMQMessageSHM(const size_t size);
    FairMQMessageSHM(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr);
    FairMQMessageSHM(FairMQUnmanagedRegionPtr& region, void* data, const size_t size);

    FairMQMessageSHM(const FairMQMessageSHM&) = delete;
    FairMQMessageSHM operator=(const FairMQMessageSHM&) = delete;

    bool InitializeChunk(const size_t size);

    virtual void Rebuild();
    virtual void Rebuild(const size_t size);
    virtual void Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr);

    virtual void* GetMessage();
    virtual void* GetData();
    virtual size_t GetSize();

    virtual void SetMessage(void* data, const size_t size);

    virtual void SetDeviceId(const std::string& deviceId);

    virtual FairMQ::Transport GetType() const;

    virtual void Copy(const std::unique_ptr<FairMQMessage>& msg);

    void CloseMessage();

    virtual ~FairMQMessageSHM();

  private:
    zmq_msg_t fMessage;
    bool fQueued;
    bool fMetaCreated;
    static std::atomic<bool> fInterrupted;
    static FairMQ::Transport fTransportType;
    uint64_t fRegionId;
    bipc::managed_shared_memory::handle_t fHandle;
    size_t fSize;
    void* fLocalPtr;
    boost::interprocess::mapped_region* fRemoteRegion; // cache region ptr
};

#endif /* FAIRMQMESSAGESHM_H_ */

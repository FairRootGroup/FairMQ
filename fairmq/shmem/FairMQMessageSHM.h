/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQMESSAGESHM_H_
#define FAIRMQMESSAGESHM_H_

#include <cstddef>
#include <string>
#include <atomic>

#include <zmq.h>

#include "FairMQMessage.h"
#include "FairMQShmManager.h"

class FairMQMessageSHM : public FairMQMessage
{
    friend class FairMQSocketSHM;

  public:
    FairMQMessageSHM();
    FairMQMessageSHM(const size_t size);
    FairMQMessageSHM(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr);
    FairMQMessageSHM(const FairMQMessageSHM&) = delete;
    FairMQMessageSHM operator=(const FairMQMessageSHM&) = delete;

    void InitializeChunk(const size_t size);

    virtual void Rebuild();
    virtual void Rebuild(const size_t size);
    virtual void Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr);

    virtual void* GetMessage();
    virtual void* GetData();
    virtual size_t GetSize();

    virtual void SetMessage(void* data, const size_t size);

    virtual void SetDeviceId(const std::string& deviceId);

    virtual void Copy(const std::unique_ptr<FairMQMessage>& msg);

    void CloseMessage();

    virtual ~FairMQMessageSHM();

    static void StringDeleter(void* data, void* str);

  private:
    zmq_msg_t fMessage;
    FairMQ::shmem::ShPtrOwner* fOwner;
    static uint64_t fMessageID;
    static std::string fDeviceID;
    bool fReceiving;
    bool fQueued;
    static std::atomic<bool> fInterrupted;
};

#endif /* FAIRMQMESSAGESHM_H_ */

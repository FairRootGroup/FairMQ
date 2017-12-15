/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQMessageZMQ.h
 *
 * @since 2014-01-17
 * @author A. Rybalchenko
 */

#ifndef FAIRMQMESSAGEZMQ_H_
#define FAIRMQMESSAGEZMQ_H_

#include <cstddef>
#include <string>
#include <memory>

#include <zmq.h>

#include "FairMQMessage.h"
#include "FairMQUnmanagedRegion.h"

class FairMQSocketZMQ;

class FairMQMessageZMQ : public FairMQMessage
{
    friend class FairMQSocketZMQ;

  public:
    FairMQMessageZMQ();
    FairMQMessageZMQ(const size_t size);
    FairMQMessageZMQ(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr);
    FairMQMessageZMQ(FairMQUnmanagedRegionPtr& region, void* data, const size_t size, void* hint = 0);

    void Rebuild() override;
    void Rebuild(const size_t size) override;
    void Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override;

    void* GetData() const override;
    size_t GetSize() const override;

    bool SetUsedSize(const size_t size) override;
    void ApplyUsedSize();

    FairMQ::Transport GetType() const override;

    void Copy(const FairMQMessagePtr& msg) override;
    void Copy(const FairMQMessage& msg) override;

    ~FairMQMessageZMQ() override;

  private:
    bool fUsedSizeModified;
    size_t fUsedSize;
    std::unique_ptr<zmq_msg_t> fMsg;
    std::unique_ptr<zmq_msg_t> fViewMsg; // view on a subset of fMsg (treating it as user buffer)
    static FairMQ::Transport fTransportType;

    zmq_msg_t* GetMessage() const;
    void CloseMessage();
};

#endif /* FAIRMQMESSAGEZMQ_H_ */

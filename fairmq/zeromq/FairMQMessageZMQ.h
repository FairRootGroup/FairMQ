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

class FairMQMessageZMQ : public FairMQMessage
{
  public:
    FairMQMessageZMQ();
    FairMQMessageZMQ(const size_t size);
    FairMQMessageZMQ(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr);
    FairMQMessageZMQ(FairMQUnmanagedRegionPtr& region, void* data, const size_t size);

    void Rebuild() override;
    void Rebuild(const size_t size) override;
    void Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override;

    void* GetMessage() override;
    void* GetData() override;
    size_t GetSize() const override;

    bool SetUsedSize(const size_t size) override;
    void ApplyUsedSize();

    void SetMessage(void* data, const size_t size) override;


    FairMQ::Transport GetType() const override;

    void Copy(const std::unique_ptr<FairMQMessage>& msg) override;

    void CloseMessage();

    ~FairMQMessageZMQ() override;

  private:
    bool fUsedSizeModified;
    size_t fUsedSize;
    std::unique_ptr<zmq_msg_t> fMsg;
    std::unique_ptr<zmq_msg_t> fViewMsg; // view on a subset of fMsg (treating it as user buffer)
    static FairMQ::Transport fTransportType;
};

#endif /* FAIRMQMESSAGEZMQ_H_ */

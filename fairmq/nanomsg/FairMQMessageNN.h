/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQMessageNN.h
 *
 * @since 2013-12-05
 * @author A. Rybalchenko
 */

#ifndef FAIRMQMESSAGENN_H_
#define FAIRMQMESSAGENN_H_

#include <cstddef>
#include <string>
#include <memory>

#include "FairMQMessage.h"
#include "FairMQUnmanagedRegion.h"

class FairMQSocketNN;

class FairMQMessageNN : public FairMQMessage
{
    friend class FairMQSocketNN;

  public:
    FairMQMessageNN();
    FairMQMessageNN(const size_t size);
    FairMQMessageNN(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr);
    FairMQMessageNN(FairMQUnmanagedRegionPtr& region, void* data, const size_t size);

    FairMQMessageNN(const FairMQMessageNN&) = delete;
    FairMQMessageNN operator=(const FairMQMessageNN&) = delete;

    void Rebuild() override;
    void Rebuild(const size_t size) override;
    void Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) override;

    void* GetData() override;
    size_t GetSize() const override;

    bool SetUsedSize(const size_t size) override;

    FairMQ::Transport GetType() const override;

    void Copy(const FairMQMessagePtr& msg) override;

    ~FairMQMessageNN() override;

  private:
    void* fMessage;
    size_t fSize;
    bool fReceiving;
    FairMQUnmanagedRegion* fRegionPtr;
    static FairMQ::Transport fTransportType;

    void* GetMessage();
    void CloseMessage();
    void SetMessage(void* data, const size_t size);
};

#endif /* FAIRMQMESSAGENN_H_ */

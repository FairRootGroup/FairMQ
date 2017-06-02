/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
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

#include "FairMQMessage.h"
#include "FairMQRegion.h"

class FairMQMessageNN : public FairMQMessage
{
  public:
    FairMQMessageNN();
    FairMQMessageNN(const size_t size);
    FairMQMessageNN(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr);
    FairMQMessageNN(FairMQRegionPtr& region, void* data, const size_t size);

    FairMQMessageNN(const FairMQMessageNN&) = delete;
    FairMQMessageNN operator=(const FairMQMessageNN&) = delete;

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

    virtual ~FairMQMessageNN();

    friend class FairMQSocketNN;

  private:
    void* fMessage;
    size_t fSize;
    bool fReceiving;
    bool fRegion;
    static std::string fDeviceID;
    static FairMQ::Transport fTransportType;

    void Clear();
};

#endif /* FAIRMQMESSAGENN_H_ */

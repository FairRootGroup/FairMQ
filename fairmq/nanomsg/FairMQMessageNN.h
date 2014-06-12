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

#include "FairMQMessage.h"

class FairMQMessageNN : public FairMQMessage
{
  public:
    FairMQMessageNN();
    FairMQMessageNN(size_t size);
    FairMQMessageNN(void* data, size_t size, fairmq_free_fn *ffn = NULL, void* hint = NULL);

    virtual void Rebuild();
    virtual void Rebuild(size_t size);
    virtual void Rebuild(void* data, size_t size, fairmq_free_fn *ffn = NULL, void* hint = NULL);

    virtual void* GetMessage();
    virtual void* GetData();
    virtual size_t GetSize();

    virtual void SetMessage(void* data, size_t size);

    virtual void CloseMessage() {};
    virtual void Copy(FairMQMessage* msg);

    virtual ~FairMQMessageNN();

    friend class FairMQSocketNN;

  private:
    void* fMessage;
    size_t fSize;
    bool fReceiving;

    void Clear();
};

#endif /* FAIRMQMESSAGENN_H_ */

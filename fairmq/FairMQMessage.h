/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
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

using fairmq_free_fn = void(void* data, void* hint);

class FairMQMessage
{
  public:
    virtual void Rebuild() = 0;
    virtual void Rebuild(const size_t size) = 0;
    virtual void Rebuild(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) = 0;

    virtual void* GetMessage() = 0;
    virtual void* GetData() = 0;
    virtual size_t GetSize() = 0;
    virtual void SetMessage(void* data, size_t size) = 0;

    virtual void SetDeviceId(const std::string& deviceId) = 0;

    virtual void Copy(const std::unique_ptr<FairMQMessage>& msg) = 0;

    virtual ~FairMQMessage() {};
};

using FairMQMessagePtr = std::unique_ptr<FairMQMessage>;

#endif /* FAIRMQMESSAGE_H_ */

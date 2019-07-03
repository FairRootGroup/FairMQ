/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQUNMANAGEDREGIONZMQ_H_
#define FAIRMQUNMANAGEDREGIONZMQ_H_

#include "FairMQUnmanagedRegion.h"

#include <cstddef> // size_t
#include <string>

class FairMQUnmanagedRegionZMQ final : public FairMQUnmanagedRegion
{
    friend class FairMQSocketZMQ;
    friend class FairMQMessageZMQ;

  public:
    FairMQUnmanagedRegionZMQ(const size_t size, FairMQRegionCallback callback, const std::string& path = "", int flags = 0);
    FairMQUnmanagedRegionZMQ(const FairMQUnmanagedRegionZMQ&) = delete;
    FairMQUnmanagedRegionZMQ operator=(const FairMQUnmanagedRegionZMQ&) = delete;

    virtual void* GetData() const override;
    virtual size_t GetSize() const override;

    virtual ~FairMQUnmanagedRegionZMQ();

  private:
    void* fBuffer;
    size_t fSize;
    FairMQRegionCallback fCallback;
};

#endif /* FAIRMQUNMANAGEDREGIONZMQ_H_ */
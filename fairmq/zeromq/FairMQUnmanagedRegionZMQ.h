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
class FairMQTransportFactory;

class FairMQUnmanagedRegionZMQ final : public FairMQUnmanagedRegion
{
    friend class FairMQSocketZMQ;
    friend class FairMQMessageZMQ;

  public:
    FairMQUnmanagedRegionZMQ(uint64_t id, const size_t size, int64_t userFlags, FairMQRegionCallback callback, const std::string& /* path = "" */, int /* flags = 0 */, FairMQTransportFactory* factory = nullptr);

    FairMQUnmanagedRegionZMQ(const FairMQUnmanagedRegionZMQ&) = delete;
    FairMQUnmanagedRegionZMQ operator=(const FairMQUnmanagedRegionZMQ&) = delete;

    virtual void* GetData() const override;
    virtual size_t GetSize() const override;
    uint64_t GetId() const override { return fId; }
    int64_t GetUserFlags() const { return fUserFlags; }

    virtual ~FairMQUnmanagedRegionZMQ();

  private:
    uint64_t fId;
    void* fBuffer;
    size_t fSize;
    int64_t fUserFlags;
    FairMQRegionCallback fCallback;
};

#endif /* FAIRMQUNMANAGEDREGIONZMQ_H_ */

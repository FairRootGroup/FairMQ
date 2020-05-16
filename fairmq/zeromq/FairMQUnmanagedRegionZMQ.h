/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQUNMANAGEDREGIONZMQ_H_
#define FAIRMQUNMANAGEDREGIONZMQ_H_

#include <fairmq/zeromq/Context.h>
#include <FairMQUnmanagedRegion.h>
#include <FairMQLogger.h>

#include <cstddef> // size_t
#include <string>

class FairMQTransportFactory;

class FairMQUnmanagedRegionZMQ final : public FairMQUnmanagedRegion
{
    friend class FairMQSocketZMQ;
    friend class FairMQMessageZMQ;

  public:
    FairMQUnmanagedRegionZMQ(fair::mq::zmq::Context& ctx,
                             size_t size,
                             int64_t userFlags,
                             FairMQRegionCallback callback,
                             FairMQRegionBulkCallback bulkCallback,
                             const std::string& /* path = "" */,
                             int /* flags = 0 */,
                             FairMQTransportFactory* factory = nullptr)
        : FairMQUnmanagedRegion(factory)
        , fCtx(ctx)
        , fId(fCtx.RegionCount())
        , fBuffer(malloc(size))
        , fSize(size)
        , fUserFlags(userFlags)
        , fCallback(callback)
        , fBulkCallback(bulkCallback)
    {}

    FairMQUnmanagedRegionZMQ(const FairMQUnmanagedRegionZMQ&) = delete;
    FairMQUnmanagedRegionZMQ operator=(const FairMQUnmanagedRegionZMQ&) = delete;

    virtual void* GetData() const override { return fBuffer; }
    virtual size_t GetSize() const override { return fSize; }
    uint64_t GetId() const override { return fId; }
    int64_t GetUserFlags() const { return fUserFlags; }

    virtual ~FairMQUnmanagedRegionZMQ()
    {
        LOG(debug) << "destroying region " << fId;
        fCtx.RemoveRegion(fId);
        free(fBuffer);
    }

  private:
    fair::mq::zmq::Context& fCtx;
    uint64_t fId;
    void* fBuffer;
    size_t fSize;
    int64_t fUserFlags;
    FairMQRegionCallback fCallback;
    FairMQRegionBulkCallback fBulkCallback;
};

#endif /* FAIRMQUNMANAGEDREGIONZMQ_H_ */

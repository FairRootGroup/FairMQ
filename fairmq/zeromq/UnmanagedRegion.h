/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_ZMQ_UNMANAGEDREGION_H
#define FAIR_MQ_ZMQ_UNMANAGEDREGION_H

#include <fairmq/zeromq/Context.h>
#include <FairMQUnmanagedRegion.h>
#include <FairMQLogger.h>

#include <cstddef> // size_t
#include <string>

namespace fair::mq::zmq
{

class UnmanagedRegion final : public fair::mq::UnmanagedRegion
{
    friend class Socket;
    friend class Message;

  public:
    UnmanagedRegion(Context& ctx,
                    size_t size,
                    int64_t userFlags,
                    RegionCallback callback,
                    RegionBulkCallback bulkCallback,
                    FairMQTransportFactory* factory = nullptr)
        : fair::mq::UnmanagedRegion(factory)
        , fCtx(ctx)
        , fId(fCtx.RegionCount())
        , fBuffer(malloc(size))
        , fSize(size)
        , fUserFlags(userFlags)
        , fCallback(callback)
        , fBulkCallback(bulkCallback)
    {}

    UnmanagedRegion(const UnmanagedRegion&) = delete;
    UnmanagedRegion operator=(const UnmanagedRegion&) = delete;

    virtual void* GetData() const override { return fBuffer; }
    virtual size_t GetSize() const override { return fSize; }
    uint16_t GetId() const override { return fId; }
    int64_t GetUserFlags() const { return fUserFlags; }
    void SetLinger(uint32_t /* linger */) override { LOG(debug) << "ZeroMQ UnmanagedRegion linger option not implemented. Acknowledgements are local."; }
    uint32_t GetLinger() const override { LOG(debug) << "ZeroMQ UnmanagedRegion linger option not implemented. Acknowledgements are local."; return 0; }

    virtual ~UnmanagedRegion()
    {
        LOG(debug) << "destroying region " << fId;
        fCtx.RemoveRegion(fId);
        free(fBuffer);
    }

  private:
    Context& fCtx;
    uint16_t fId;
    void* fBuffer;
    size_t fSize;
    int64_t fUserFlags;
    RegionCallback fCallback;
    RegionBulkCallback fBulkCallback;
};

} // namespace fair::mq::zmq

#endif /* FAIR_MQ_ZMQ_UNMANAGEDREGION_H */

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQUnmanagedRegionZMQ.h"
#include "FairMQLogger.h"

FairMQUnmanagedRegionZMQ::FairMQUnmanagedRegionZMQ(fair::mq::zmq::Context& ctx,
                                                   size_t size,
                                                   int64_t userFlags,
                                                   FairMQRegionCallback callback,
                                                   FairMQRegionBulkCallback bulkCallback,
                                                   const std::string& /* path = "" */,
                                                   int /* flags = 0 */,
                                                   FairMQTransportFactory* factory)
    : FairMQUnmanagedRegion(factory)
    , fCtx(ctx)
    , fId(fCtx.RegionCount())
    , fBuffer(malloc(size))
    , fSize(size)
    , fUserFlags(userFlags)
    , fCallback(callback)
    , fBulkCallback(bulkCallback)
{}

void* FairMQUnmanagedRegionZMQ::GetData() const
{
    return fBuffer;
}

size_t FairMQUnmanagedRegionZMQ::GetSize() const
{
    return fSize;
}

FairMQUnmanagedRegionZMQ::~FairMQUnmanagedRegionZMQ()
{
    LOG(debug) << "destroying region " << fId;
    fCtx.RemoveRegion(fId);
    free(fBuffer);
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQUnmanagedRegionZMQ.h"
#include "FairMQTransportFactoryZMQ.h"
#include "FairMQLogger.h"

FairMQUnmanagedRegionZMQ::FairMQUnmanagedRegionZMQ(uint64_t id, const size_t size, int64_t userFlags, FairMQRegionCallback callback, const std::string& /* path = "" */, int /* flags = 0 */, FairMQTransportFactory* factory /* = nullptr */)
    : FairMQUnmanagedRegion(factory)
    , fId(id)
    , fBuffer(malloc(size))
    , fSize(size)
    , fUserFlags(userFlags)
    , fCallback(callback)
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
    LOG(debug) << "destroying region";
    static_cast<FairMQTransportFactoryZMQ*>(GetTransport())->RemoveRegion(fId);
    free(fBuffer);
}

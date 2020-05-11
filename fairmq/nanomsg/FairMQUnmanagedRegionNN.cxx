/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQUnmanagedRegionNN.h"
#include "FairMQLogger.h"

using namespace std;

FairMQUnmanagedRegionNN::FairMQUnmanagedRegionNN(const size_t size, FairMQRegionCallback callback, const std::string& /*path = "" */, int /*flags = 0 */, FairMQTransportFactory* factory /* = nullptr */)
    : FairMQUnmanagedRegion(factory)
    , fBuffer(malloc(size))
    , fSize(size)
    , fCallback(callback)
{
}

FairMQUnmanagedRegionNN::FairMQUnmanagedRegionNN(const size_t size, const int64_t /*userFlags*/, FairMQRegionCallback callback, const std::string& /*path = "" */, int /*flags = 0 */, FairMQTransportFactory* factory /* = nullptr */)
    : FairMQUnmanagedRegion(factory)
    , fBuffer(malloc(size))
    , fSize(size)
    , fCallback(callback)
{
}

void* FairMQUnmanagedRegionNN::GetData() const
{
    return fBuffer;
}

size_t FairMQUnmanagedRegionNN::GetSize() const
{
    return fSize;
}

FairMQUnmanagedRegionNN::~FairMQUnmanagedRegionNN()
{
    LOG(debug) << "destroying region";
    free(fBuffer);
}

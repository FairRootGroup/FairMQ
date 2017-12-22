/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQUnmanagedRegionZMQ.h"
#include "FairMQLogger.h"

using namespace std;

FairMQUnmanagedRegionZMQ::FairMQUnmanagedRegionZMQ(const size_t size, FairMQRegionCallback callback)
    : fBuffer(malloc(size))
    , fSize(size)
    , fCallback(callback)
{
}

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
    free(fBuffer);
}

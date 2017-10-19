/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQUnmanagedRegionNN.h"
#include "FairMQLogger.h"

using namespace std;

FairMQUnmanagedRegionNN::FairMQUnmanagedRegionNN(const size_t size)
    : fBuffer(malloc(size))
    , fSize(size)
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
    LOG(DEBUG) << "destroying region";
    free(fBuffer);
}

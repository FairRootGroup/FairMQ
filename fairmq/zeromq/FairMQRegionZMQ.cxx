/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQRegionZMQ.h"
#include "FairMQLogger.h"

using namespace std;

FairMQRegionZMQ::FairMQRegionZMQ(const size_t size)
    : fBuffer(malloc(size))
    , fSize(size)
{
}

void* FairMQRegionZMQ::GetData() const
{
    return fBuffer;
}

size_t FairMQRegionZMQ::GetSize() const
{
    return fSize;
}

FairMQRegionZMQ::~FairMQRegionZMQ()
{
    LOG(DEBUG) << "destroying region";
    free(fBuffer);
}

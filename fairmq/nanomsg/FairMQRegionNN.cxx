/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQRegionNN.h"
#include "FairMQLogger.h"

using namespace std;

FairMQRegionNN::FairMQRegionNN(const size_t size)
    : fBuffer(malloc(size))
    , fSize(size)
{
}

void* FairMQRegionNN::GetData() const
{
    return fBuffer;
}

size_t FairMQRegionNN::GetSize() const
{
    return fSize;
}

FairMQRegionNN::~FairMQRegionNN()
{
    LOG(DEBUG) << "destroying region";
    free(fBuffer);
}

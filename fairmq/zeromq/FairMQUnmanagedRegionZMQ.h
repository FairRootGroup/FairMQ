/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQUNMANAGEDREGIONZMQ_H_
#define FAIRMQUNMANAGEDREGIONZMQ_H_

#include "FairMQUnmanagedRegion.h"

#include <cstddef> // size_t

class FairMQUnmanagedRegionZMQ : public FairMQUnmanagedRegion
{
    friend class FairMQSocketSHM;

  public:
    FairMQUnmanagedRegionZMQ(const size_t size);
    FairMQUnmanagedRegionZMQ(const FairMQUnmanagedRegionZMQ&) = delete;
    FairMQUnmanagedRegionZMQ operator=(const FairMQUnmanagedRegionZMQ&) = delete;

    virtual void* GetData() const override;
    virtual size_t GetSize() const override;

    virtual ~FairMQUnmanagedRegionZMQ();

  private:
    void* fBuffer;
    size_t fSize;
};

#endif /* FAIRMQUNMANAGEDREGIONZMQ_H_ */
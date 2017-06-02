/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQREGIONZMQ_H_
#define FAIRMQREGIONZMQ_H_

#include "FairMQRegion.h"

#include <cstddef> // size_t

class FairMQRegionZMQ : public FairMQRegion
{
    friend class FairMQSocketSHM;

  public:
    FairMQRegionZMQ(const size_t size);
    FairMQRegionZMQ(const FairMQRegionZMQ&) = delete;
    FairMQRegionZMQ operator=(const FairMQRegionZMQ&) = delete;

    virtual void* GetData() const override;
    virtual size_t GetSize() const override;

    virtual ~FairMQRegionZMQ();

  private:
    void* fBuffer;
    size_t fSize;
};

#endif /* FAIRMQREGIONZMQ_H_ */
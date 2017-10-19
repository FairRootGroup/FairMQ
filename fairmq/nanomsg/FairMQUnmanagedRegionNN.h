/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQUNMANAGEDREGIONNN_H_
#define FAIRMQUNMANAGEDREGIONNN_H_

#include "FairMQUnmanagedRegion.h"

#include <cstddef> // size_t

class FairMQUnmanagedRegionNN : public FairMQUnmanagedRegion
{
    friend class FairMQSocketNN;

  public:
    FairMQUnmanagedRegionNN(const size_t size);
    FairMQUnmanagedRegionNN(const FairMQUnmanagedRegionNN&) = delete;
    FairMQUnmanagedRegionNN operator=(const FairMQUnmanagedRegionNN&) = delete;

    virtual void* GetData() const override;
    virtual size_t GetSize() const override;

    virtual ~FairMQUnmanagedRegionNN();

  private:
    void* fBuffer;
    size_t fSize;
};

#endif /* FAIRMQUNMANAGEDREGIONNN_H_ */
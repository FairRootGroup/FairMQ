/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQREGIONNN_H_
#define FAIRMQREGIONNN_H_

#include "FairMQRegion.h"

#include <cstddef> // size_t

class FairMQRegionNN : public FairMQRegion
{
    friend class FairMQSocketNN;

  public:
    FairMQRegionNN(const size_t size);
    FairMQRegionNN(const FairMQRegionNN&) = delete;
    FairMQRegionNN operator=(const FairMQRegionNN&) = delete;

    virtual void* GetData() const override;
    virtual size_t GetSize() const override;

    virtual ~FairMQRegionNN();

  private:
    void* fBuffer;
    size_t fSize;
};

#endif /* FAIRMQREGIONNN_H_ */
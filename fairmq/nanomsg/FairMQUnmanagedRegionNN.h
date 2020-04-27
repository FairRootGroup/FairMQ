/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQUNMANAGEDREGIONNN_H_
#define FAIRMQUNMANAGEDREGIONNN_H_

#include "FairMQUnmanagedRegion.h"

#include <cstddef> // size_t
#include <string>

class FairMQUnmanagedRegionNN final : public FairMQUnmanagedRegion
{
    friend class FairMQSocketNN;

  public:
    FairMQUnmanagedRegionNN(const size_t size, FairMQRegionCallback callback, const std::string& path = "", int flags = 0);
    FairMQUnmanagedRegionNN(const size_t size, const int64_t userFlags, FairMQRegionCallback callback, const std::string& path = "", int flags = 0);

    FairMQUnmanagedRegionNN(const FairMQUnmanagedRegionNN&) = delete;
    FairMQUnmanagedRegionNN operator=(const FairMQUnmanagedRegionNN&) = delete;

    virtual void* GetData() const override;
    virtual size_t GetSize() const override;

    virtual ~FairMQUnmanagedRegionNN();

  private:
    void* fBuffer;
    size_t fSize;
    FairMQRegionCallback fCallback;
};

#endif /* FAIRMQUNMANAGEDREGIONNN_H_ */

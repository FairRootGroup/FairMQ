/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQUNMANAGEDREGION_H_
#define FAIRMQUNMANAGEDREGION_H_

#include <cstddef> // size_t
#include <memory> // unique_ptr

class FairMQUnmanagedRegion
{
  public:
    virtual void* GetData() const = 0;
    virtual size_t GetSize() const = 0;

    virtual ~FairMQUnmanagedRegion() {};
};

using FairMQUnmanagedRegionPtr = std::unique_ptr<FairMQUnmanagedRegion>;

#endif /* FAIRMQUNMANAGEDREGION_H_ */

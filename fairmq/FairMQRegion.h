/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQREGION_H_
#define FAIRMQREGION_H_

#include <cstddef> // size_t
#include <memory> // unique_ptr

class FairMQRegion
{
  public:
    virtual void* GetData() const = 0;
    virtual size_t GetSize() const = 0;

    virtual ~FairMQRegion() {};
};

using FairMQRegionPtr = std::unique_ptr<FairMQRegion>;

#endif /* FAIRMQREGION_H_ */

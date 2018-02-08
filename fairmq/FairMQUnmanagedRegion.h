/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQUNMANAGEDREGION_H_
#define FAIRMQUNMANAGEDREGION_H_

#include <cstddef> // size_t
#include <memory> // std::unique_ptr
#include <functional> // std::function

using FairMQRegionCallback = std::function<void(void*, size_t, void*)>;

class FairMQUnmanagedRegion
{
  public:
    virtual void* GetData() const = 0;
    virtual size_t GetSize() const = 0;

    virtual ~FairMQUnmanagedRegion() {};
};

using FairMQUnmanagedRegionPtr = std::unique_ptr<FairMQUnmanagedRegion>;

namespace fair
{
namespace mq
{

using UnmanagedRegionPtr = std::unique_ptr<FairMQUnmanagedRegion>;

} /* namespace mq */
} /* namespace fair */

#endif /* FAIRMQUNMANAGEDREGION_H_ */

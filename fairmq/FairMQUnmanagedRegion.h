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
#include <ostream> // std::ostream

enum class FairMQRegionEvent : int
{
    created,
    destroyed
};

struct FairMQRegionInfo {
    uint64_t id;   // id of the region
    void* ptr;     // pointer to the start of the region
    size_t size;   // region size
    int64_t flags; // custom flags set by the creator
    FairMQRegionEvent event;
};

using FairMQRegionCallback = std::function<void(void*, size_t, void*)>;
using FairMQRegionEventCallback = std::function<void(FairMQRegionInfo)>;

class FairMQUnmanagedRegion
{
  public:
    virtual void* GetData() const = 0;
    virtual size_t GetSize() const = 0;

    virtual ~FairMQUnmanagedRegion() {};
};

using FairMQUnmanagedRegionPtr = std::unique_ptr<FairMQUnmanagedRegion>;

inline std::ostream& operator<<(std::ostream& os, const FairMQRegionEvent& event)
{
    if (event == FairMQRegionEvent::created) {
        return os << "created";
    } else {
        return os << "destroyed";
    }
}

namespace fair
{
namespace mq
{

using RegionCallback = FairMQRegionCallback;
using RegionEventCallback = FairMQRegionEventCallback;
using RegionEvent = FairMQRegionEvent;
using RegionInfo = FairMQRegionInfo;
using UnmanagedRegion = FairMQUnmanagedRegion;
using UnmanagedRegionPtr = FairMQUnmanagedRegionPtr;

} /* namespace mq */
} /* namespace fair */

#endif /* FAIRMQUNMANAGEDREGION_H_ */

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

class FairMQTransportFactory;

enum class FairMQRegionEvent : int
{
    created,
    destroyed,
    local_only
};

struct FairMQRegionInfo
{
    FairMQRegionInfo()
        : id(0)
        , ptr(nullptr)
        , size(0)
        , flags(0)
        , event(FairMQRegionEvent::created)
    {}

    FairMQRegionInfo(uint64_t _id, void* _ptr, size_t _size, int64_t _flags, FairMQRegionEvent _event)
        : id(_id)
        , ptr(_ptr)
        , size(_size)
        , flags(_flags)
        , event (_event)
    {}

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
    FairMQUnmanagedRegion() {}
    FairMQUnmanagedRegion(FairMQTransportFactory* factory): fTransport(factory) {}

    virtual void* GetData() const = 0;
    virtual size_t GetSize() const = 0;

    FairMQTransportFactory* GetTransport() { return fTransport; }
    void SetTransport(FairMQTransportFactory* transport) { fTransport = transport; }

    virtual ~FairMQUnmanagedRegion() {};

  private:
    FairMQTransportFactory* fTransport{nullptr};
};

using FairMQUnmanagedRegionPtr = std::unique_ptr<FairMQUnmanagedRegion>;

inline std::ostream& operator<<(std::ostream& os, const FairMQRegionEvent& event)
{
    switch (event) {
        case FairMQRegionEvent::created:
            return os << "created";
        case FairMQRegionEvent::destroyed:
            return os << "destroyed";
        case FairMQRegionEvent::local_only:
            return os << "local_only";
        default:
            return os << "unrecognized event";
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

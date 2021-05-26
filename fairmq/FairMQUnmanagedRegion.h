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
#include <cstdint> // uint32_t
#include <memory> // std::unique_ptr
#include <functional> // std::function
#include <ostream> // std::ostream
#include <vector>

class FairMQTransportFactory;

enum class FairMQRegionEvent : int
{
    created,
    destroyed,
    local_only
};

struct FairMQRegionInfo
{
    FairMQRegionInfo() = default;

    FairMQRegionInfo(bool _managed, uint64_t _id, void* _ptr, size_t _size, int64_t _flags, FairMQRegionEvent _event)
        : managed(_managed)
        , id(_id)
        , ptr(_ptr)
        , size(_size)
        , flags(_flags)
        , event(_event)
    {}

    bool managed = true;  // managed/unmanaged
    uint64_t id = 0;   // id of the region
    void* ptr = nullptr;     // pointer to the start of the region
    size_t size = 0;   // region size
    int64_t flags = 0; // custom flags set by the creator
    FairMQRegionEvent event = FairMQRegionEvent::created;
};

struct FairMQRegionBlock {
    void* ptr;
    size_t size;
    void* hint;

    FairMQRegionBlock(void* p, size_t s, void* h)
        : ptr(p), size(s), hint(h)
    {}
};

using FairMQRegionCallback = std::function<void(void*, size_t, void*)>;
using FairMQRegionBulkCallback = std::function<void(const std::vector<FairMQRegionBlock>&)>;
using FairMQRegionEventCallback = std::function<void(FairMQRegionInfo)>;

class FairMQUnmanagedRegion
{
  public:
    FairMQUnmanagedRegion() = default;
    FairMQUnmanagedRegion(FairMQTransportFactory* factory) : fTransport(factory) {}

    virtual void* GetData() const = 0;
    virtual size_t GetSize() const = 0;
    virtual uint16_t GetId() const = 0;
    virtual void SetLinger(uint32_t linger) = 0;
    virtual uint32_t GetLinger() const = 0;

    virtual fair::mq::Transport GetType() const = 0;
    FairMQTransportFactory* GetTransport() { return fTransport; }
    void SetTransport(FairMQTransportFactory* transport) { fTransport = transport; }

    virtual ~FairMQUnmanagedRegion() = default;

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

namespace fair::mq
{

struct RegionConfig
{
    RegionConfig() = default;

    RegionConfig(bool l, bool z)
        : lock(l), zero(z)
    {}

    bool lock = false;
    bool zero = false;
};

using RegionCallback = FairMQRegionCallback;
using RegionBulkCallback = FairMQRegionBulkCallback;
using RegionEventCallback = FairMQRegionEventCallback;
using RegionEvent = FairMQRegionEvent;
using RegionInfo = FairMQRegionInfo;
using RegionBlock = FairMQRegionBlock;
using UnmanagedRegion = FairMQUnmanagedRegion;
using UnmanagedRegionPtr = FairMQUnmanagedRegionPtr;

} // namespace fair::mq

#endif /* FAIRMQUNMANAGEDREGION_H_ */

/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_UNMANAGEDREGION_H
#define FAIR_MQ_UNMANAGEDREGION_H

#include <fairmq/Transports.h>

#include <cstddef>   // size_t
#include <cstdint>   // uint32_t

#include <functional>   // std::function
#include <memory>       // std::unique_ptr
#include <optional>     // std::optional
#include <ostream>
#include <string>
#include <vector>

namespace fair::mq {

class TransportFactory;

enum class RegionEvent : int
{
    created,
    destroyed,
    local_only
};

struct RegionInfo
{
    RegionInfo() = default;

    RegionInfo(bool _managed,
               uint64_t _id,
               void* _ptr,
               size_t _size,
               int64_t _flags,
               RegionEvent _event)
        : managed(_managed)
        , id(_id)
        , ptr(_ptr)
        , size(_size)
        , flags(_flags)
        , event(_event)
    {}

    bool managed = true;   // managed/unmanaged
    uint64_t id = 0;       // id of the region
    void* ptr = nullptr;   // pointer to the start of the region
    size_t size = 0;       // region size
    int64_t flags = 0;     // custom flags set by the creator
    RegionEvent event = RegionEvent::created;
};

struct RegionBlock
{
    void* ptr;
    size_t size;
    void* hint;

    RegionBlock(void* p, size_t s, void* h)
        : ptr(p)
        , size(s)
        , hint(h)
    {}
};

using RegionCallback = std::function<void(void*, size_t, void*)>;
using RegionBulkCallback = std::function<void(const std::vector<RegionBlock>&)>;
using RegionEventCallback = std::function<void(RegionInfo)>;

struct UnmanagedRegion
{
    UnmanagedRegion() = default;
    UnmanagedRegion(TransportFactory* factory)
        : fTransport(factory)
    {}

    UnmanagedRegion(const UnmanagedRegion&) = delete;
    UnmanagedRegion(UnmanagedRegion&&) = delete;
    UnmanagedRegion& operator=(const UnmanagedRegion&) = delete;
    UnmanagedRegion& operator=(UnmanagedRegion&&) = delete;

    virtual void* GetData() const = 0;
    virtual size_t GetSize() const = 0;
    virtual uint16_t GetId() const = 0;
    virtual void SetLinger(uint32_t linger) = 0;
    virtual uint32_t GetLinger() const = 0;

    virtual Transport GetType() const = 0;
    TransportFactory* GetTransport() { return fTransport; }
    void SetTransport(TransportFactory* transport) { fTransport = transport; }

    virtual ~UnmanagedRegion() = default;

  private:
    TransportFactory* fTransport{nullptr};
};

using UnmanagedRegionPtr = std::unique_ptr<UnmanagedRegion>;

inline std::ostream& operator<<(std::ostream& os, const RegionEvent& event)
{
    switch (event) {
        case RegionEvent::created:
            return os << "created";
        case RegionEvent::destroyed:
            return os << "destroyed";
        case RegionEvent::local_only:
            return os << "local_only";
        default:
            return os << "unrecognized event";
    }
}

enum class RegionConstruction : int
{
    open,
    create,
    open_or_create
};

struct RegionConfig
{
    RegionConfig() = default;

    RegionConfig(bool _lock, bool _zero)
        : lock(_lock)
        , zero(_zero)
    {}

    bool lock = false; /// mlock region after creation
    bool zero = false; /// zero region content after creation
    bool removeOnDestruction = true; /// remove the region on object destruction
    RegionConstruction constructionMode = RegionConstruction::create; /// how to construct the region: create/open/open_or_create
    int creationFlags = 0; /// flags passed to the underlying transport on region creation
    int64_t userFlags = 0; /// custom flags that have no effect on the transport, but can be retrieved from the region by the user
    std::string path = ""; /// file path, if the region is backed by a file
    std::optional<uint16_t> id = std::nullopt; /// region id
    uint32_t linger = 100; /// delay in ms before region destruction to collect outstanding events
};

}   // namespace fair::mq

// using FairMQRegionEvent [[deprecated("Use fair::mq::RegionBlock")]] = fair::mq::RegionEvent;
// using FairMQRegionInfo [[deprecated("Use fair::mq::RegionInfo")]] = fair::mq::RegionInfo;
// using FairMQRegionBlock [[deprecated("Use fair::mq::RegionBlock")]] = fair::mq::RegionBlock;
// using FairMQRegionCallback [[deprecated("Use fair::mq::RegionCallback")]] = fair::mq::RegionCallback;
// using FairMQRegionBulkCallback [[deprecated("Use fair::mq::RegionBulkCallback")]] = fair::mq::RegionBulkCallback;
// using FairMQRegionEventCallback [[deprecated("Use fair::mq::RegionEventCallback")]] = fair::mq::RegionEventCallback;
// using FairMQUnmanagedRegion [[deprecated("Use fair::mq::UnmanagedRegion")]] = fair::mq::UnmanagedRegion;
// using FairMQUnmanagedRegionPtr [[deprecated("Use fair::mq::UnmanagedRegionPtr")]] = fair::mq::UnmanagedRegionPtr;
// using FairMQRegionConfig [[deprecated("Use fair::mq::RegionConfig")]] = fair::mq::RegionConfig;
using FairMQRegionEvent = fair::mq::RegionEvent;
using FairMQRegionInfo = fair::mq::RegionInfo;
using FairMQRegionBlock = fair::mq::RegionBlock;
using FairMQRegionCallback = fair::mq::RegionCallback;
using FairMQRegionBulkCallback = fair::mq::RegionBulkCallback;
using FairMQRegionEventCallback = fair::mq::RegionEventCallback;
using FairMQUnmanagedRegion = fair::mq::UnmanagedRegion;
using FairMQUnmanagedRegionPtr = fair::mq::UnmanagedRegionPtr;
using FairMQRegionConfig = fair::mq::RegionConfig;

#endif   // FAIR_MQ_UNMANAGEDREGION_H

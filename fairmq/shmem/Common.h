/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_SHMEM_COMMON_H_
#define FAIR_MQ_SHMEM_COMMON_H_

#include <picosha2.h>

#include <atomic>
#include <string>
#include <unordered_map>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/functional/hash.hpp>

#include <unistd.h>
#include <sys/types.h>

namespace fair
{
namespace mq
{
namespace shmem
{

using SegmentManager = boost::interprocess::managed_shared_memory::segment_manager;
using VoidAlloc      = boost::interprocess::allocator<void, SegmentManager>;
using CharAlloc      = boost::interprocess::allocator<char, SegmentManager>;
using Str            = boost::interprocess::basic_string<char, std::char_traits<char>, CharAlloc>;
using StrAlloc       = boost::interprocess::allocator<Str, SegmentManager>;
using StrVector      = boost::interprocess::vector<Str, StrAlloc>;

struct RegionInfo
{
    RegionInfo(const VoidAlloc& alloc)
        : fPath("", alloc)
        , fFlags(0)
        , fUserFlags(0)
        , fDestroyed(false)
    {}

    RegionInfo(const char* path, const int flags, const uint64_t userFlags, const VoidAlloc& alloc)
        : fPath(path, alloc)
        , fFlags(flags)
        , fUserFlags(userFlags)
        , fDestroyed(false)
    {}

    Str fPath;
    int fFlags;
    uint64_t fUserFlags;
    bool fDestroyed;
};

using Uint64RegionInfoPairAlloc = boost::interprocess::allocator<std::pair<const uint64_t, RegionInfo>, SegmentManager>;
using Uint64RegionInfoMap = boost::interprocess::map<uint64_t, RegionInfo, std::less<uint64_t>, Uint64RegionInfoPairAlloc>;

struct DeviceCounter
{
    DeviceCounter(unsigned int c)
        : fCount(c)
    {}

    std::atomic<unsigned int> fCount;
};

struct RegionCounter
{
    RegionCounter(uint64_t c)
        : fCount(c)
    {}

    std::atomic<uint64_t> fCount;
};

struct MetaHeader
{
    size_t fSize;
    size_t fRegionId;
    size_t fHint;
    boost::interprocess::managed_shared_memory::handle_t fHandle;
};

struct RegionBlock
{
    RegionBlock()
        : fHandle()
        , fSize(0)
        , fHint(0)
    {}

    RegionBlock(boost::interprocess::managed_shared_memory::handle_t handle, size_t size, size_t hint)
        : fHandle(handle)
        , fSize(size)
        , fHint(hint)
    {}

    boost::interprocess::managed_shared_memory::handle_t fHandle;
    size_t fSize;
    size_t fHint;
};

// find id for unique shmem name:
// a hash of user id + session id, truncated to 8 characters (to accommodate for name size limit on some systems (MacOS)).
inline std::string buildShmIdFromSessionIdAndUserId(const std::string& sessionId)
{
    std::string seed((std::to_string(geteuid()) + sessionId));
    std::string shmId = picosha2::hash256_hex_string(seed);
    shmId.resize(10, '_');
    return shmId;
}

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_COMMON_H_ */

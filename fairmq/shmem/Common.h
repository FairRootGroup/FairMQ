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
#include <functional> // std::equal_to

#include <boost/functional/hash.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/unordered_map.hpp>

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
using Uint64RegionInfoHashMap = boost::unordered_map<uint64_t, RegionInfo, boost::hash<uint64_t>, std::equal_to<uint64_t>, Uint64RegionInfoPairAlloc>;

struct DeviceCounter
{
    DeviceCounter(unsigned int c)
        : fCount(c)
    {}

    std::atomic<unsigned int> fCount;
};

#ifdef FAIRMQ_DEBUG_MODE
struct MsgCounter
{
    MsgCounter(unsigned int c)
        : fCount(c)
    {}

    std::atomic<unsigned int> fCount;
};
#endif

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

#ifdef FAIRMQ_DEBUG_MODE
struct MsgDebug
{
    MsgDebug(pid_t pid, size_t size, const uint64_t creationTime)
        : fPid(pid)
        , fSize(size)
        , fCreationTime(creationTime)
    {}

    pid_t fPid;
    size_t fSize;
    uint64_t fCreationTime;
};

using SizetMsgDebugPairAlloc = boost::interprocess::allocator<std::pair<const size_t, MsgDebug>, SegmentManager>;
using SizetMsgDebugHashMap = boost::unordered_map<size_t, MsgDebug, boost::hash<size_t>, std::equal_to<size_t>, SizetMsgDebugPairAlloc>;
using SizetMsgDebugMap = boost::interprocess::map<size_t, MsgDebug, std::less<size_t>, SizetMsgDebugPairAlloc>;
#endif

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
    // generate a 8-digit hex value out of sha256 hash
    std::vector<unsigned char> hash(4);
    picosha2::hash256(seed.begin(), seed.end(), hash.begin(), hash.end());
    std::string shmId = picosha2::bytes_to_hex_string(hash.begin(), hash.end());

    return shmId;
}

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_COMMON_H_ */

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_SHMEM_COMMON_H_
#define FAIR_MQ_SHMEM_COMMON_H_

#include <atomic>
#include <string>
#include <functional> // std::equal_to

#include <boost/functional/hash.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/indexes/null_index.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/mem_algo/simple_seq_fit.hpp>
#include <boost/unordered_map.hpp>
#include <boost/variant.hpp>

#include <sys/types.h>

namespace fair::mq::shmem
{

struct SharedMemoryError : std::runtime_error { using std::runtime_error::runtime_error; };

using SimpleSeqFitSegment = boost::interprocess::basic_managed_shared_memory<char,
    boost::interprocess::simple_seq_fit<boost::interprocess::mutex_family, boost::interprocess::offset_ptr<void>>,
    boost::interprocess::null_index>;
    // boost::interprocess::iset_index>;
using RBTreeBestFitSegment = boost::interprocess::basic_managed_shared_memory<char,
    boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family, boost::interprocess::offset_ptr<void>>,
    boost::interprocess::null_index>;
    // boost::interprocess::iset_index>;

using SegmentManager = boost::interprocess::managed_shared_memory::segment_manager;
using VoidAlloc      = boost::interprocess::allocator<void, SegmentManager>;
using CharAlloc      = boost::interprocess::allocator<char, SegmentManager>;
using Str            = boost::interprocess::basic_string<char, std::char_traits<char>, CharAlloc>;
using StrAlloc       = boost::interprocess::allocator<Str, SegmentManager>;
using StrVector      = boost::interprocess::vector<Str, StrAlloc>;

enum class AllocationAlgorithm : int
{
    rbtree_best_fit,
    simple_seq_fit
};

struct RegionInfo
{
    RegionInfo(const VoidAlloc& alloc)
        : fPath("", alloc)
        , fCreationFlags(0)
        , fUserFlags(0)
        , fDestroyed(false)
    {}

    RegionInfo(const char* path, const int flags, const uint64_t userFlags, const VoidAlloc& alloc)
        : fPath(path, alloc)
        , fCreationFlags(flags)
        , fUserFlags(userFlags)
        , fDestroyed(false)
    {}

    Str fPath;
    int fCreationFlags;
    uint64_t fUserFlags;
    bool fDestroyed;
};

using Uint16RegionInfoPairAlloc = boost::interprocess::allocator<std::pair<const uint16_t, RegionInfo>, SegmentManager>;
using Uint16RegionInfoMap = boost::interprocess::map<uint16_t, RegionInfo, std::less<uint16_t>, Uint16RegionInfoPairAlloc>;
using Uint16RegionInfoHashMap = boost::unordered_map<uint16_t, RegionInfo, boost::hash<uint16_t>, std::equal_to<uint16_t>, Uint16RegionInfoPairAlloc>;

struct SegmentInfo
{
    SegmentInfo(AllocationAlgorithm aa)
        : fAllocationAlgorithm(aa)
    {}

    AllocationAlgorithm fAllocationAlgorithm;
};

struct SessionInfo
{
    SessionInfo(const char* sessionName, int creatorId, const VoidAlloc& alloc)
        : fSessionName(sessionName, alloc)
        , fCreatorId(creatorId)
    {}

    Str fSessionName;
    int fCreatorId;
};

using Uint16SegmentInfoPairAlloc = boost::interprocess::allocator<std::pair<const uint16_t, SegmentInfo>, SegmentManager>;
using Uint16SegmentInfoHashMap = boost::unordered_map<uint16_t, SegmentInfo, boost::hash<uint16_t>, std::equal_to<uint16_t>, Uint16SegmentInfoPairAlloc>;
// using Uint16SegmentInfoMap = boost::interprocess::map<uint16_t, SegmentInfo, std::less<uint16_t>, Uint16SegmentInfoPairAlloc>;

struct DeviceCounter
{
    DeviceCounter(unsigned int c)
        : fCount(c)
    {}

    std::atomic<unsigned int> fCount;
};

struct EventCounter
{
    EventCounter(uint64_t c)
        : fCount(c)
    {}

    std::atomic<uint64_t> fCount;
};

struct Heartbeat
{
    Heartbeat(uint64_t c)
        : fCount(c)
    {}

    std::atomic<uint64_t> fCount;
};

struct RegionCounter
{
    RegionCounter(uint16_t c)
        : fCount(c)
    {}

    std::atomic<uint16_t> fCount;
};

struct MetaHeader
{
    size_t fSize;
    size_t fHint;
    boost::interprocess::managed_shared_memory::handle_t fHandle;
    mutable boost::interprocess::managed_shared_memory::handle_t fShared;
    uint16_t fRegionId;
    mutable uint16_t fSegmentId;
    bool fManaged;
};

#ifdef FAIRMQ_DEBUG_MODE
struct MsgCounter
{
    MsgCounter()
        : fCount(0)
    {}

    MsgCounter(unsigned int c)
        : fCount(c)
    {}

    std::atomic<unsigned int> fCount;
};

using Uint16MsgCounterPairAlloc = boost::interprocess::allocator<std::pair<const uint16_t, MsgCounter>, SegmentManager>;
using Uint16MsgCounterHashMap = boost::unordered_map<uint16_t, MsgCounter, boost::hash<uint16_t>, std::equal_to<uint16_t>, Uint16MsgCounterPairAlloc>;

struct MsgDebug
{
    MsgDebug()
        : fPid(0)
        , fSize(0)
        , fCreationTime(0)
    {}

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
// using SizetMsgDebugHashMap = boost::unordered_map<size_t, MsgDebug, boost::hash<size_t>, std::equal_to<size_t>, SizetMsgDebugPairAlloc>;
using SizetMsgDebugMap = boost::interprocess::map<size_t, MsgDebug, std::less<size_t>, SizetMsgDebugPairAlloc>;
using Uint16MsgDebugMapPairAlloc = boost::interprocess::allocator<std::pair<const uint16_t, SizetMsgDebugMap>, SegmentManager>;
using Uint16MsgDebugMapHashMap = boost::unordered_map<uint16_t, SizetMsgDebugMap, boost::hash<uint16_t>, std::equal_to<uint16_t>, Uint16MsgDebugMapPairAlloc>;
#endif

struct RegionBlock
{
    RegionBlock() = default;

    RegionBlock(boost::interprocess::managed_shared_memory::handle_t handle, size_t size, size_t hint)
        : fHandle(handle)
        , fSize(size)
        , fHint(hint)
    {}

    boost::interprocess::managed_shared_memory::handle_t fHandle{};
    size_t fSize = 0;
    size_t fHint = 0;
};

// find id for unique shmem name:
// a hash of user id + session id, truncated to 8 characters (to accommodate for name size limit on some systems (MacOS)).
std::string makeShmIdStr(const std::string& sessionId, const std::string& userId);
std::string makeShmIdStr(const std::string& sessionId);
std::string makeShmIdStr(uint64_t val);
uint64_t makeShmIdUint64(const std::string& sessionId);


struct SegmentSize : public boost::static_visitor<size_t>
{
    template<typename S>
    size_t operator()(S& s) const { return s.get_size(); }
};

struct SegmentAddress : public boost::static_visitor<void*>
{
    template<typename S>
    void* operator()(S& s) const { return s.get_address(); }
};

struct SegmentMemoryZeroer : public boost::static_visitor<>
{
    template<typename S>
    void operator()(S& s) const { s.zero_free_memory(); }
};

struct SegmentFreeMemory : public boost::static_visitor<size_t>
{
    template<typename S>
    size_t operator()(S& s) const { return s.get_free_memory(); }
};

struct SegmentHandleFromAddress : public boost::static_visitor<boost::interprocess::managed_shared_memory::handle_t>
{
    SegmentHandleFromAddress(const void* _ptr) : ptr(_ptr) {}

    template<typename S>
    boost::interprocess::managed_shared_memory::handle_t operator()(S& s) const { return s.get_handle_from_address(ptr); }

    const void* ptr;
};

struct SegmentAddressFromHandle : public boost::static_visitor<char*>
{
    SegmentAddressFromHandle(const boost::interprocess::managed_shared_memory::handle_t _handle) : handle(_handle) {}

    template<typename S>
    char* operator()(S& s) const { return reinterpret_cast<char*>(s.get_address_from_handle(handle)); }

    const boost::interprocess::managed_shared_memory::handle_t handle;
};

struct SegmentAllocate : public boost::static_visitor<char*>
{
    SegmentAllocate(const size_t _size) : size(_size) {}

    template<typename S>
    char* operator()(S& s) const { return reinterpret_cast<char*>(s.allocate(size)); }

    const size_t size;
};

struct SegmentAllocateAligned : public boost::static_visitor<void*>
{
    SegmentAllocateAligned(const size_t _size, const size_t _alignment) : size(_size), alignment(_alignment) {}

    template<typename S>
    void* operator()(S& s) const { return s.allocate_aligned(size, alignment); }

    const size_t size;
    const size_t alignment;
};

struct SegmentBufferShrink : public boost::static_visitor<char*>
{
    SegmentBufferShrink(const size_t _new_size, char* _local_ptr)
        : new_size(_new_size)
        , local_ptr(_local_ptr)
    {}

    template<typename S>
    char* operator()(S& s) const
    {
        boost::interprocess::managed_shared_memory::size_type shrunk_size = new_size;
        return s.template allocation_command<char>(boost::interprocess::shrink_in_place, new_size + 128, shrunk_size, local_ptr);
    }

    const size_t new_size;
    mutable char* local_ptr;
};

struct SegmentDeallocate : public boost::static_visitor<>
{
    SegmentDeallocate(char* _ptr) : ptr(_ptr) {}

    template<typename S>
    void operator()(S& s) const { return s.deallocate(ptr); }

    char* ptr;
};

} // namespace fair::mq::shmem

#endif /* FAIR_MQ_SHMEM_COMMON_H_ */

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

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/functional/hash.hpp>

#include <unistd.h>
#include <sys/types.h>

namespace fair
{
namespace mq
{
namespace shmem
{

struct DeviceCounter
{
    DeviceCounter(unsigned int c)
        : fCount(c)
    {}

    std::atomic<unsigned int> fCount;
};

struct RegionCounter
{
    RegionCounter(unsigned int c)
        : fCount(c)
    {}

    std::atomic<unsigned int> fCount;
};

struct MonitorStatus
{
    MonitorStatus()
        : fActive(true)
    {}

    bool fActive;
};

struct MetaHeader
{
    size_t fSize;
    size_t fRegionId;
    boost::interprocess::managed_shared_memory::handle_t fHandle;
    size_t fHint;
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
    boost::hash<std::string> stringHash;
    std::string shmId(std::to_string(stringHash(std::string((std::to_string(geteuid()) + sessionId)))));
    shmId.resize(8, '_');
    return shmId;
}

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_COMMON_H_ */

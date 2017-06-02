/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_SHMEM_COMMON_H_
#define FAIR_MQ_SHMEM_COMMON_H_

#include <atomic>

#include <boost/interprocess/managed_shared_memory.hpp>

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

struct alignas(32) MetaHeader
{
    uint64_t fSize;
    uint64_t fRegionId;
    boost::interprocess::managed_shared_memory::handle_t fHandle;
};

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_SHMEM_COMMON_H_ */

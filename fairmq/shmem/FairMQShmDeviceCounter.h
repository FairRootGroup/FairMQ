/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQSHMDEVICECOUNTER_H_
#define FAIRMQSHMDEVICECOUNTER_H_

#include <atomic>

namespace fair
{
namespace mq
{
namespace shmem
{

struct DeviceCounter
{
    DeviceCounter(unsigned int c)
        : count(c)
    {}

    std::atomic<unsigned int> count;
};

} // namespace shmem
} // namespace mq
} // namespace fair

#endif /* FAIRMQSHMDEVICECOUNTER_H_ */

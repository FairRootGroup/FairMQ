/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/ProgOptions.h>
#include <fairmq/shmem/Monitor.h>
#include <fairmq/tools/Unique.h>
#include <fairmq/TransportFactory.h>

#include <gtest/gtest.h>

#include <string>

namespace
{

using namespace std;
using namespace fair::mq;

void GetFreeMemory()
{
    ProgOptions config;
    string sessionId(to_string(tools::UuidHash()));
    config.SetProperty<string>("session", sessionId);

    ASSERT_THROW(shmem::Monitor::GetFreeMemory(shmem::SessionId{sessionId}, 0), shmem::Monitor::MonitorError);

    auto factory = TransportFactory::CreateTransportFactory("shmem", tools::Uuid(), &config);

    ASSERT_NO_THROW(shmem::Monitor::GetFreeMemory(shmem::SessionId{sessionId}, 0));
    ASSERT_THROW(shmem::Monitor::GetFreeMemory(shmem::SessionId{sessionId}, 1), shmem::Monitor::MonitorError);
}

TEST(Monitor, GetFreeMemory)
{
    GetFreeMemory();
}

} // namespace

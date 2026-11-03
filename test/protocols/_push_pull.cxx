/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <helper/Devices.h>
#include <helper/Transport.h>

#include <gtest/gtest.h>

#include <string>

namespace
{

using namespace std;
using namespace fair::mq::test;

auto RunPushPull(string const& transport, string const& extraArgs) -> void
{
    IpcAddress const address("push_pull");

    auto const results = RunDevices({
        {"push_" + transport, "name=data,type=push,method=bind,address=" + address.str()},
        {"pull_" + transport, "name=data,type=pull,method=connect,address=" + address.str()},
    }, RunOptions(transport, extraArgs));

    for (auto const& result : results) {
        EXPECT_TRUE(Succeeded(result, transport));
    }
}

TEST(PushPull, SingleMsg_MultiThreaded_ipc_zeromq)
{
    RunPushPull("zeromq", "");
}

TEST(PushPull, SingleMsg_MultiThreaded_ipc_shmem)
{
    RunPushPull("shmem", "");
}

TEST(PushPull, SingleMsg_MultiThreaded_ipc_shmem_expanded_metadata)
{
    RunPushPull("shmem", "--shm-metadata-msg-size 2048");
}

} // namespace

/********************************************************************************
 * Copyright (C) 2018-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
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

auto RunPair(string const& transport, string const& extraArgs) -> void
{
    IpcAddress const address("pair");

    auto const results = RunDevices({
        {"pairleft_" + transport, "name=data,type=pair,method=bind,address=" + address.str()},
        {"pairright_" + transport, "name=data,type=pair,method=connect,address=" + address.str()},
    }, RunOptions(transport, extraArgs));

    for (auto const& result : results) {
        EXPECT_TRUE(Succeeded(result, transport));
    }
}

TEST(Pair, SingleMsg_MultiThreaded_ipc_zeromq)
{
    RunPair("zeromq", "");
}

TEST(Pair, SingleMsg_MultiThreaded_ipc_shmem)
{
    RunPair("shmem", "");
}

TEST(Pair, SingleMsg_MultiThreaded_ipc_shmem_expanded_metadata)
{
    RunPair("shmem", "--shm-metadata-msg-size 2048");
}

} // namespace

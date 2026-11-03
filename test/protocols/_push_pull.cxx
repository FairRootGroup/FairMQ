/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <helper/Devices.h>
#include <helper/Transport.h>
#include <helper/TransportParam.h>

#include <gtest/gtest.h>

#include <string>

namespace
{

using namespace std;
using namespace fair::mq::test;

using PushPull = TransportTest;

TEST_P(PushPull, SingleMsg_MultiThreaded_ipc)
{
    auto const& transport = GetParam().transport;
    IpcAddress const address("push_pull");

    auto const results = RunDevices({
        {"push_" + transport, "name=data,type=push,method=bind,address=" + address.str()},
        {"pull_" + transport, "name=data,type=pull,method=connect,address=" + address.str()},
    }, RunOptions(transport, GetParam().DeviceArgs()));

    for (auto const& result : results) {
        EXPECT_TRUE(Succeeded(result, transport));
    }
}

INSTANTIATE_TEST_SUITE_P(, PushPull, AllTransports(), TransportParamName());

} // namespace

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

using PubSub = TransportTest;

TEST_P(PubSub, MultiThreaded_ipc)
{
    auto const& transport = GetParam().transport;
    IpcAddress const data("pub_sub_data");
    IpcAddress const control("pub_sub_control");

    auto const subChannels = "name=data,type=sub,method=connect,address=" + data.str()
                           + " name=control,type=push,method=connect,address=" + control.str();
    auto const results = RunDevices({
        {"pub_" + transport, "name=data,type=pub,method=bind,address=" + data.str()
                             + " name=control,type=pull,method=bind,address=" + control.str()},
        {"sub_1_" + transport, subChannels},
        {"sub_2_" + transport, subChannels},
    }, RunOptions(transport, GetParam().DeviceArgs()));

    for (auto const& result : results) {
        EXPECT_TRUE(Succeeded(result, transport));
    }
}

// The shmem transport does not support pub/sub.
INSTANTIATE_TEST_SUITE_P(, PubSub, testing::Values(TransportParam("zeromq")), TransportParamName());

} // namespace

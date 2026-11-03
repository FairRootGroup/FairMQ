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

using ReqRep = TransportTest;

TEST_P(ReqRep, MultiThreaded_ipc)
{
    auto const& transport = GetParam().transport;
    IpcAddress const address("req_rep");

    auto const results = RunDevices({
        {"rep_" + transport, "name=data,type=rep,method=bind,address=" + address.str()},
        {"req_1_" + transport, "name=data,type=req,method=connect,address=" + address.str()},
        {"req_2_" + transport, "name=data,type=req,method=connect,address=" + address.str()},
    }, RunOptions(transport, GetParam().DeviceArgs()));

    for (auto const& result : results) {
        EXPECT_TRUE(Succeeded(result, transport));
    }
}

INSTANTIATE_TEST_SUITE_P(, ReqRep, AllTransports(), TransportParamName());

} // namespace

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

auto RunPoller(string const& transport, int pollType) -> void
{
    IpcAddress const data1("poller_data1");
    IpcAddress const data2("poller_data2");

    auto const results = RunDevices({
        {"pollout_" + transport, "name=data1,type=push,method=bind,address=" + data1.str()
                                 + " name=data2,type=push,method=bind,address=" + data2.str()},
        {"pollin_" + transport, "name=data1,type=pull,method=connect,address=" + data1.str()
                                + " name=data2,type=pull,method=connect,address=" + data2.str(),
         "--poll-type " + to_string(pollType)},
    }, RunOptions(transport));

    for (auto const& result : results) {
        EXPECT_TRUE(Succeeded(result, transport));
    }
}

TEST(Subchannel, zeromq)
{
    RunPoller("zeromq", 0);
}

TEST(Subchannel, shmem)
{
    RunPoller("shmem", 0);
}

TEST(Channel, zeromq)
{
    RunPoller("zeromq", 1);
}

TEST(Channel, shmem)
{
    RunPoller("shmem", 1);
}

} // namespace

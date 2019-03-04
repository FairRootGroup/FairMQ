/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "runner.h"
#include <fairmq/Tools.h>
#include <gtest/gtest.h>
#include <sstream> // std::stringstream
#include <thread>

namespace
{

using namespace std;
using namespace fair::mq::test;
using namespace fair::mq::tools;

auto RunPoller(string transport, int pollType) -> void
{
    size_t session{fair::mq::tools::UuidHash()};

    auto pollout = execute_result{"", 0};
    thread poll_out_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id pollout_"<< transport
            << " --control static --color false"
            << " --session " << session << " --mq-config \"" << mqConfig << "\"";
        pollout = execute(cmd.str(), "[POLLOUT]");
    });

    auto pollin = execute_result{"", 0};
    thread poll_in_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id pollin_" << transport
            << " --control static --color false"
            << " --session " << session << " --mq-config \"" << mqConfig << "\" --poll-type " << pollType;
        pollin = execute(cmd.str(), "[POLLIN]");
    });

    poll_out_thread.join();
    poll_in_thread.join();
    cerr << pollout.console_out << pollin.console_out;

    exit(pollout.exit_code + pollin.exit_code);
}

TEST(Subchannel, zeromq)
{
    EXPECT_EXIT(RunPoller("zeromq", 0), ::testing::ExitedWithCode(0), "POLL test successfull");
}

#ifdef BUILD_NANOMSG_TRANSPORT
TEST(Subchannel, nanomsg)
{
    EXPECT_EXIT(RunPoller("nanomsg", 0), ::testing::ExitedWithCode(0), "POLL test successfull");
}
#endif /* BUILD_NANOMSG_TRANSPORT */

TEST(Subchannel, shmem)
{
    EXPECT_EXIT(RunPoller("shmem", 0), ::testing::ExitedWithCode(0), "POLL test successfull");
}

TEST(Channel, zeromq)
{
    EXPECT_EXIT(RunPoller("zeromq", 1), ::testing::ExitedWithCode(0), "POLL test successfull");
}

#ifdef BUILD_NANOMSG_TRANSPORT
TEST(Channel, nanomsg)
{
    EXPECT_EXIT(RunPoller("nanomsg", 1), ::testing::ExitedWithCode(0), "POLL test successfull");
}
#endif /* BUILD_NANOMSG_TRANSPORT */

TEST(Channel, shmem)
{
    EXPECT_EXIT(RunPoller("shmem", 1), ::testing::ExitedWithCode(0), "POLL test successfull");
}

} // namespace

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

auto RunPoller(string transport, int pollType) -> void
{
    size_t session{fair::mq::tools::UuidHash()};

    auto pollout = execute_result{"", 0};
    thread poll_out_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id pollout_"<< transport
            << " --control static --verbosity DEBUG --log-color false"
            << " --session " << session << " --mq-config \"" << mqConfig << "\"";
        pollout = execute(cmd.str(), "[POLLOUT]");
    });

    auto pollin = execute_result{"", 0};
    thread poll_in_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice
            << " --id pollin_" << transport
            << " --control static --verbosity DEBUG --log-color false"
            << " --session " << session << " --mq-config \"" << mqConfig << "\" --poll-type " << pollType;
        pollin = execute(cmd.str(), "[POLLIN]");
    });

    poll_out_thread.join();
    poll_in_thread.join();
    cerr << pollout.error_out << pollin.error_out;

    exit(pollout.exit_code + pollin.exit_code);
}

TEST(Poller, ZeroMQ_subchannel)
{
    EXPECT_EXIT(RunPoller("zeromq", 0), ::testing::ExitedWithCode(0), "POLL test successfull");
}

#ifdef NANOMSG_FOUND
TEST(Poller, Nanomsg_subchannel)
{
    EXPECT_EXIT(RunPoller("nanomsg", 0), ::testing::ExitedWithCode(0), "POLL test successfull");
}
#endif /* NANOMSG_FOUND */

TEST(Poller, ShMem_subchannel)
{
    EXPECT_EXIT(RunPoller("shmem", 0), ::testing::ExitedWithCode(0), "POLL test successfull");
}

TEST(Poller, ZeroMQ_channel)
{
    EXPECT_EXIT(RunPoller("zeromq", 1), ::testing::ExitedWithCode(0), "POLL test successfull");
}

#ifdef NANOMSG_FOUND
TEST(Poller, Nanomsg_channel)
{
    EXPECT_EXIT(RunPoller("nanomsg", 1), ::testing::ExitedWithCode(0), "POLL test successfull");
}
#endif /* NANOMSG_FOUND */

TEST(Poller, ShMem_channel)
{
    EXPECT_EXIT(RunPoller("shmem", 1), ::testing::ExitedWithCode(0), "POLL test successfull");
}

} // namespace

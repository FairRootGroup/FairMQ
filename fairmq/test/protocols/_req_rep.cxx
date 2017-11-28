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

auto RunReqRep(string transport) -> void
{
    size_t session{fair::mq::tools::UuidHash()};

    auto rep = execute_result{ "", 0 };
    thread rep_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice << " --id rep_" << transport << " --control static --verbosity DEBUG "
        << "--session " << session << " --log-color false --mq-config \"" << mqConfig << "\"";
        rep = execute(cmd.str(), "[REP]");
    });

    auto req1 = execute_result{ "", 0 };
    thread req1_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice << " --id req_1" << transport << " --control static --verbosity DEBUG "
        << "--session " << session << " --log-color false --mq-config \"" << mqConfig << "\"";
        req1 = execute(cmd.str(), "[REQ1]");
    });

    auto req2 = execute_result{ "", 0 };
    thread req2_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice << " --id req_2" << transport << " --control static --verbosity DEBUG "
        << "--session " << session << " --log-color false --mq-config \"" << mqConfig << "\"";
        req2 = execute(cmd.str(), "[REQ2]");
    });

    rep_thread.join();
    req1_thread.join();
    req2_thread.join();
    cerr << req1.error_out << req2.error_out << rep.error_out;

    exit(req1.exit_code + req2.exit_code + rep.exit_code);
}

TEST(ReqRep, ZeroMQ)
{
    EXPECT_EXIT(RunReqRep("zeromq"), ::testing::ExitedWithCode(0), "REQ-REP test successfull");
}

TEST(ReqRep, ShMem)
{
    EXPECT_EXIT(RunReqRep("shmem"), ::testing::ExitedWithCode(0), "REQ-REP test successfull");
}

#ifdef NANOMSG_FOUND
TEST(ReqRep, Nanomsg)
{
    EXPECT_EXIT(RunReqRep("nanomsg"), ::testing::ExitedWithCode(0), "REQ-REP test successfull");
}
#endif /* NANOMSG_FOUND */

} // namespace

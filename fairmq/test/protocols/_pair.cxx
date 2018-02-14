/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
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

auto RunPair(string transport) -> void
{
    size_t session{fair::mq::tools::UuidHash()};

    auto pairleft = execute_result{"", 100};
    thread pairleft_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice << " --id pairleft_" << transport << " --control static "
        << "--session " << session << " --color false --mq-config \"" << mqConfig << "\"";
        pairleft = execute(cmd.str(), "[PAIR L]");
    });

    auto pairright = execute_result{"", 100};
    thread pairright_thread([&]() {
        stringstream cmd;
        cmd << runTestDevice << " --id pairright_" << transport << " --control static "
        << "--session " << session << " --color false --mq-config \"" << mqConfig << "\"";
        pairright = execute(cmd.str(), "[PAIR R]");
    });

    pairleft_thread.join();
    pairright_thread.join();
    cerr << pairleft.console_out << pairright.console_out;

    exit(pairleft.exit_code + pairright.exit_code);
}

TEST(Pair, MP_ZeroMQ__tcp____SingleMsg)
{
    EXPECT_EXIT(RunPair("zeromq"), ::testing::ExitedWithCode(0), "PAIR test successfull");
}

TEST(Pair, MP_ShMem___tcp____SingleMsg)
{
    EXPECT_EXIT(RunPair("shmem"), ::testing::ExitedWithCode(0), "PAIR test successfull");
}

#ifdef NANOMSG_FOUND
TEST(Pair, MP_Nanomsg_tcp____SingleMsg)
{
    EXPECT_EXIT(RunPair("nanomsg"), ::testing::ExitedWithCode(0), "PAIR test successfull");
}
#endif /* NANOMSG_FOUND */

TEST(Pair, MP_Ofi_____tcp____SingleMsg)
{
    EXPECT_EXIT(RunPair("ofi"), ::testing::ExitedWithCode(0), "PAIR test successfull");
}

} // namespace

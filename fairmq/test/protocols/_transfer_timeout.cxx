/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * @file fairmq/test/protocols/_transfer_timeout.cxx
 */

#include "runner.h"
#include <gtest/gtest.h>
#include <sstream> // std::stringstream

namespace
{

using namespace std;
using namespace fair::mq::test;

auto RunTransferTimeout(string transport) -> void
{
    stringstream cmd;
    cmd << runTestDevice << " --id transfer_timeout_" << transport << " --control static --verbosity DEBUG "
        << "--log-color false --mq-config \"" << mqConfig << "\"";
    auto res = execute(cmd.str());

    cerr << res.error_out;

    exit(res.exit_code);
}

TEST(TransferTimeout, ZeroMQ)
{
    EXPECT_EXIT(RunTransferTimeout("zeromq"), ::testing::ExitedWithCode(0), "Transfer timeout test successfull");
}

TEST(TransferTimeout, ShMem)
{
    EXPECT_EXIT(RunTransferTimeout("shmem"), ::testing::ExitedWithCode(0), "Transfer timeout test successfull");
}

#ifdef NANOMSG_FOUND
TEST(TransferTimeout, Nanomsg)
{
    EXPECT_EXIT(RunTransferTimeout("nanomsg"), ::testing::ExitedWithCode(0), "Transfer timeout test successfull");
}
#endif /* NANOMSG_FOUND */

} // namespace

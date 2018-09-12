/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "runner.h"

#include <gtest/gtest.h>
#include <boost/process.hpp>

#include <sys/types.h>
#include <signal.h>

#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <future> // std::async, std::future

namespace
{

using namespace std;
using namespace fair::mq::test;

void RunWaitFor()
{
    std::mutex mtx;
    std::condition_variable cv;

    int pid = 0;
    int exit_code = 0;

    thread deviceThread([&]() {
        stringstream cmd;
        cmd << runTestDevice << " --id waitfor_" << " --control static " << " --severity nolog";

        boost::process::ipstream stdout;
        boost::process::child c(cmd.str(), boost::process::std_out > stdout);
        string line;
        getline(stdout, line);

        {
            std::lock_guard<std::mutex> lock(mtx);
            pid = c.id();
        }
        cv.notify_one();

        c.wait();

        exit_code = c.exit_code();
    });

    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&pid]{ return pid != 0; });
    }

    kill(pid, SIGINT);

    deviceThread.join();

    exit(exit_code);
}

TEST(Device, WaitFor)
{
    EXPECT_EXIT(RunWaitFor(), ::testing::ExitedWithCode(0), "");
}

} // namespace

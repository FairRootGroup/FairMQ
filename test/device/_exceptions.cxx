/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <helper/Devices.h>

#include <gtest/gtest.h>

#include <string>

namespace
{

using namespace std;
using namespace fair::mq::test;

void RunExceptionIn(string const& state, string const& control, string const& input = "")
{
    RunOptions options;
    options.control = control;
    options.input = input;
    auto const results = RunDevices({{"exceptions_" + state + "_"}}, options);

    EXPECT_TRUE(ExitedWith(results.at(0), 1, "exception in " + state + "()"));
}

TEST(Exceptions, static_InInit)
{
    RunExceptionIn("Init", "static");
}

TEST(Exceptions, static_InBind)
{
    RunExceptionIn("Bind", "static");
}

TEST(Exceptions, static_InConnect)
{
    RunExceptionIn("Connect", "static");
}

TEST(Exceptions, static_InInitTask)
{
    RunExceptionIn("InitTask", "static");
}

TEST(Exceptions, static_InPreRun)
{
    RunExceptionIn("PreRun", "static");
}

TEST(Exceptions, static_InRun)
{
    RunExceptionIn("Run", "static");
}

TEST(Exceptions, static_InPostRun)
{
    RunExceptionIn("PostRun", "static");
}

TEST(Exceptions, static_InResetTask)
{
    RunExceptionIn("ResetTask", "static");
}

TEST(Exceptions, static_InReset)
{
    RunExceptionIn("Reset", "static");
}

TEST(Exceptions, interactive_InInit)
{
    RunExceptionIn("Init", "interactive");
}

TEST(Exceptions, interactive_InBind)
{
    RunExceptionIn("Bind", "interactive");
}

TEST(Exceptions, interactive_InConnect)
{
    RunExceptionIn("Connect", "interactive");
}

TEST(Exceptions, interactive_InInitTask)
{
    RunExceptionIn("InitTask", "interactive");
}

TEST(Exceptions, interactive_InPreRun)
{
    RunExceptionIn("PreRun", "interactive");
}

TEST(Exceptions, interactive_InRun)
{
    RunExceptionIn("Run", "interactive");
}

TEST(Exceptions, interactive_InPostRun)
{
    RunExceptionIn("PostRun", "interactive");
}

TEST(Exceptions, interactive_InResetTask)
{
    RunExceptionIn("ResetTask", "interactive", "q");
}

TEST(Exceptions, interactive_InReset)
{
    RunExceptionIn("Reset", "interactive", "q");
}

} // namespace

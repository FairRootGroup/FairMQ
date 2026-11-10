/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <helper/Devices.h>
#include <helper/StateParam.h>

#include <gtest/gtest.h>

#include <string>

namespace
{

using namespace std;
using namespace fair::mq::test;

using Exceptions = StateTest;

TEST_P(Exceptions, Throw)
{
    auto const& state = GetParam().state;
    RunOptions options;
    options.control = GetParam().control;
    options.input = GetParam().Input();
    auto const results = RunDevices({{"exceptions_" + state + "_"}}, options);

    EXPECT_TRUE(ExitedWith(results.at(0), 1, "exception in " + state + "()"));
}

INSTANTIATE_TEST_SUITE_P(, Exceptions, WithControls(HookStates()), StateParamName());

} // namespace

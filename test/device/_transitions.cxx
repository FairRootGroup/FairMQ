/********************************************************************************
 * Copyright (C) 2014-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairlogger/Logger.h>

#include <gtest/gtest.h>

#include <vector>
#include <thread>

namespace
{

using namespace std;
using namespace fair::mq;

TEST(Transitions, InvalidChangeState)
{
    Device device;
    thread t([&] { device.RunStateMachine(); });

    ASSERT_FALSE(device.ChangeState(Transition::Connect));

    ASSERT_TRUE(device.ChangeState(Transition::End));
    if (t.joinable()) { t.join(); }
}

TEST(Transitions, InvalidChangeStateOrThrow)
{
    Device device;
    thread t([&] { device.RunStateMachine(); });

    ASSERT_THROW(device.ChangeStateOrThrow(Transition::Connect), std::system_error);

    ASSERT_NO_THROW(device.ChangeStateOrThrow(Transition::End));
    if (t.joinable()) { t.join(); }
}

} // namespace

/********************************************************************************
 * Copyright (C) 2014-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "../helper/ControlDevice.h"

#include <fairmq/Device.h>
#include <fairlogger/Logger.h>

#include <gtest/gtest.h>

#include <system_error>

namespace
{

using namespace std;
using namespace fair::mq;

TEST(Transitions, InvalidChangeState)
{
    Device device;
    test::StateMachineThread t(device);

    ASSERT_FALSE(device.ChangeState(Transition::Connect));

    ASSERT_TRUE(device.ChangeState(Transition::End));
    t.Join();
}

TEST(Transitions, InvalidChangeStateOrThrow)
{
    Device device;
    test::StateMachineThread t(device);

    ASSERT_THROW(device.ChangeStateOrThrow(Transition::Connect), std::system_error);

    ASSERT_NO_THROW(device.ChangeStateOrThrow(Transition::End));
    t.Join();
}

} // namespace

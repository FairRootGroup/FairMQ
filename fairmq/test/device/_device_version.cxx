/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "TestVersion.h"

#include <fairmq/Tools.h>

#include <gtest/gtest.h>

#include <sstream> // std::stringstream
#include <thread>

namespace
{

using namespace std;
using namespace fair::mq::test;

class DeviceVersion : public ::testing::Test {
  public:
    DeviceVersion()
    {}

    fair::mq::tools::Version TestDeviceVersion()
    {
        fair::mq::test::TestVersion versionDevice({1, 2, 3});
        versionDevice.ChangeState("END");

        return versionDevice.GetVersion();
    }
};

TEST_F(DeviceVersion, getter)
{
    struct fair::mq::tools::Version v{1, 2, 3};
    fair::mq::tools::Version version = TestDeviceVersion();

    EXPECT_EQ(v, version);
}

} // namespace

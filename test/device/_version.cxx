/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQDevice.h>

#include <fairmq/tools/Version.h>

#include <gtest/gtest.h>

#include <sstream> // std::stringstream
#include <thread>

namespace
{

using namespace std;

class TestVersion : public FairMQDevice
{
  public:
    TestVersion(fair::mq::tools::Version version)
        : FairMQDevice(version)
    {}
};

class Version : public ::testing::Test
{
  public:
    Version()
    {}

    fair::mq::tools::Version TestDeviceVersion()
    {
        TestVersion versionDevice({1, 2, 3});
        versionDevice.ChangeState("END");

        return versionDevice.GetVersion();
    }
};

TEST_F(Version, getter)
{
    struct fair::mq::tools::Version v{1, 2, 3};
    fair::mq::tools::Version version = TestDeviceVersion();

    EXPECT_EQ(v, version);
}

} // namespace

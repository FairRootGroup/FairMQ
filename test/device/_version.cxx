/********************************************************************************
 * Copyright (C) 2014-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>

#include <fairmq/tools/Version.h>

#include <gtest/gtest.h>

#include <sstream> // std::stringstream
#include <thread>

namespace
{

using namespace std;

class TestVersion : public fair::mq::Device
{
  public:
    TestVersion(fair::mq::tools::Version version)
        : fair::mq::Device(version)
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
        versionDevice.ChangeStateOrThrow("END");

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

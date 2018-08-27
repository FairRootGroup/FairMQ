/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQDevice.h>
#include <options/FairMQProgOptions.h>

#include <fairmq/Tools.h>

#include <gtest/gtest.h>

#include <sstream> // std::stringstream
#include <thread>

namespace
{

using namespace std;

void control(FairMQDevice& device)
{
    device.ChangeState("INIT_DEVICE");
    device.WaitForEndOfState("INIT_DEVICE");
    device.ChangeState("INIT_TASK");
    device.WaitForEndOfState("INIT_TASK");

    device.ChangeState("RUN");
    device.WaitForEndOfState("RUN");

    device.ChangeState("RESET_TASK");
    device.WaitForEndOfState("RESET_TASK");
    device.ChangeState("RESET_DEVICE");
    device.WaitForEndOfState("RESET_DEVICE");

    device.ChangeState("END");
}

class DeviceConfig : public ::testing::Test
{
  public:
    DeviceConfig()
    {}

    string TestDeviceSetConfig(const string& transport)
    {
        FairMQProgOptions config;

        vector<string> emptyArgs = {"dummy", "--id", "test", "--color", "false"};

        if (config.ParseAll(emptyArgs, true))
        {
            return 0;
        }

        config.SetValue<string>("transport", transport);

        FairMQDevice device;
        device.SetConfig(config);

        FairMQChannel channel;
        channel.UpdateType("pub");
        channel.UpdateMethod("connect");
        channel.UpdateAddress("tcp://localhost:5558");
        device.AddChannel("data", channel);

        thread t(control, ref(device));

        device.RunStateMachine();

        if (t.joinable())
        {
            t.join();
        }

        return device.GetTransportName();
    }

    string TestDeviceSetTransport(const string& transport)
    {
        FairMQDevice device;
        device.SetTransport(transport);

        FairMQChannel channel;
        channel.UpdateType("pub");
        channel.UpdateMethod("connect");
        channel.UpdateAddress("tcp://localhost:5558");
        device.AddChannel("data", channel);

        std::thread t(control, std::ref(device));

        device.RunStateMachine();

        if (t.joinable())
        {
            t.join();
        }

        return device.GetTransportName();
    }
};

TEST_F(DeviceConfig, SetConfig)
{
    string transport = "zeromq";
    string returnedTransport = TestDeviceSetConfig(transport);

    EXPECT_EQ(transport, returnedTransport);
}

TEST_F(DeviceConfig, SetTransport)
{
    string transport = "zeromq";
    string returnedTransport = TestDeviceSetTransport(transport);

    EXPECT_EQ(transport, returnedTransport);
}

} // namespace

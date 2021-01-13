/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQDevice.h>
#include <fairmq/DeviceRunner.h>
#include <fairmq/ProgOptions.h>

#include <gtest/gtest.h>

#include <sstream> // std::stringstream
#include <thread>

namespace _config
{

using namespace std;

void control(FairMQDevice& device)
{
    device.ChangeState(fair::mq::Transition::InitDevice);
    device.WaitForState(fair::mq::State::InitializingDevice);
    device.ChangeState(fair::mq::Transition::CompleteInit);
    device.WaitForState(fair::mq::State::Initialized);
    device.ChangeState(fair::mq::Transition::Bind);
    device.WaitForState(fair::mq::State::Bound);
    device.ChangeState(fair::mq::Transition::Connect);
    device.WaitForState(fair::mq::State::DeviceReady);
    device.ChangeState(fair::mq::Transition::InitTask);
    device.WaitForState(fair::mq::State::Ready);

    device.ChangeState(fair::mq::Transition::Run);
    device.WaitForState(fair::mq::State::Ready);

    device.ChangeState(fair::mq::Transition::ResetTask);
    device.WaitForState(fair::mq::State::DeviceReady);
    device.ChangeState(fair::mq::Transition::ResetDevice);
    device.WaitForState(fair::mq::State::Idle);

    device.ChangeState(fair::mq::Transition::End);
}

class TestDevice : public FairMQDevice
{
  public:
    TestDevice(const string& transport)
    {
        fDeviceThread = thread(&FairMQDevice::RunStateMachine, this);

        SetTransport(transport);

        ChangeState(fair::mq::Transition::InitDevice);
        WaitForState(fair::mq::State::InitializingDevice);
        ChangeState(fair::mq::Transition::CompleteInit);
        WaitForState(fair::mq::State::Initialized);
        ChangeState(fair::mq::Transition::Bind);
        WaitForState(fair::mq::State::Bound);
        ChangeState(fair::mq::Transition::Connect);
        WaitForState(fair::mq::State::DeviceReady);
        ChangeState(fair::mq::Transition::InitTask);
        WaitForState(fair::mq::State::Ready);

        ChangeState(fair::mq::Transition::Run);
    }

    ~TestDevice()
    {
        WaitForState(fair::mq::State::Running);
        ChangeState(fair::mq::Transition::Stop);
        WaitForState(fair::mq::State::Ready);
        ChangeState(fair::mq::Transition::ResetTask);
        WaitForState(fair::mq::State::DeviceReady);
        ChangeState(fair::mq::Transition::ResetDevice);
        WaitForState(fair::mq::State::Idle);

        ChangeState(fair::mq::Transition::End);

        if (fDeviceThread.joinable()) {
            fDeviceThread.join();
        }
    }

  private:
    thread fDeviceThread;
};

class Config : public ::testing::Test
{
  public:
    Config()
    {}

    string TestDeviceSetConfig(const string& transport)
    {
        fair::mq::ProgOptions config;

        vector<string> emptyArgs = {"dummy", "--id", "test", "--color", "false"};

        config.ParseAll(emptyArgs, true);

        config.SetProperty("transport", transport);

        FairMQDevice device;
        device.SetConfig(config);

        FairMQChannel channel;
        channel.UpdateType("pub");
        channel.UpdateMethod("connect");
        channel.UpdateAddress("tcp://localhost:5558");
        device.AddChannel("data", std::move(channel));

        thread t(control, ref(device));

        device.RunStateMachine();

        if (t.joinable()) {
            t.join();
        }

        return device.GetTransportName();
    }

    string TestDeviceSetConfigWithPlugins(const string& transport)
    {
        fair::mq::ProgOptions config;

        vector<string> emptyArgs = {"dummy", "--id", "test", "--color", "false"};

        config.SetProperty("transport", transport);

        FairMQDevice device;
        fair::mq::PluginManager mgr;
        mgr.LoadPlugin("s:config");
        mgr.ForEachPluginProgOptions([&](boost::program_options::options_description options) {
            config.AddToCmdLineOptions(options);
        });
        mgr.EmplacePluginServices(config, device);
        mgr.InstantiatePlugins();

        config.ParseAll(emptyArgs, true);
        fair::mq::DeviceRunner::HandleGeneralOptions(config);
        device.SetConfig(config);

        FairMQChannel channel;
        channel.UpdateType("pub");
        channel.UpdateMethod("connect");
        channel.UpdateAddress("tcp://localhost:5558");
        device.AddChannel("data", std::move(channel));

        thread t(control, ref(device));

        device.RunStateMachine();

        config.PrintOptions();

        if (t.joinable()) {
            t.join();
        }

        return device.GetTransportName();
    }

    string TestDeviceControlInConstructor(const string& transport)
    {
        TestDevice device(transport);

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
        device.AddChannel("data", std::move(channel));

        thread t(&FairMQDevice::RunStateMachine, &device);

        control(device);

        if (t.joinable()) {
            t.join();
        }

        return device.GetTransportName();
    }
};

TEST_F(Config, SetConfig)
{
    string transport = "zeromq";
    string returnedTransport = TestDeviceSetConfig(transport);

    EXPECT_EQ(transport, returnedTransport);
}

TEST_F(Config, SetConfigWithPlugins)
{
    string transport = "zeromq";
    string returnedTransport = TestDeviceSetConfigWithPlugins(transport);

    EXPECT_EQ(transport, returnedTransport);
}

TEST_F(Config, SetTransport)
{
    string transport = "zeromq";
    string returnedTransport = TestDeviceSetTransport(transport);

    EXPECT_EQ(transport, returnedTransport);
}

TEST_F(Config, ControlInConstructor)
{
    string transport = "zeromq";
    string returnedTransport = TestDeviceControlInConstructor(transport);

    EXPECT_EQ(transport, returnedTransport);
}

} // namespace

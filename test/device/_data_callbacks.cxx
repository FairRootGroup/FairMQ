/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Channel.h>
#include <fairmq/Device.h>
#include <fairmq/Parts.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/TransportFactory.h>
#include <fairmq/tools/Unique.h>

#include <helper/ControlDevice.h>
#include <helper/Sync.h>
#include <helper/Transport.h>
#include <helper/TransportParam.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace
{

using namespace std;
using namespace fair::mq;

auto AsString(Message const& msg) -> string
{
    return {static_cast<char const*>(msg.GetData()), msg.GetSize()};
}

/// A device with in-process control, and peer channels for the test.
class DataCallbacks : public test::TransportTest
{
  protected:
    DataCallbacks()
        : fSession(test::Configure(fConfig, tools::Uuid(), GetParam().expandedShmMetadata))
    {
        fConfig.SetProperty<string>("id", "receiver");
        fConfig.SetProperty<string>("transport", GetParam().transport);
        test::Configure(fPeerConfig, fSession, GetParam().expandedShmMetadata);
    }

    /// Add a bound input channel to the device.
    void AddInput(Device& device, string const& name, test::IpcAddress const& address, string const& transport = "")
    {
        Channel channel("pull", "bind", address);
        channel.UpdateRateLogging(0);
        if (!transport.empty()) {
            channel.UpdateTransport(transport);
        }
        device.AddChannel(name, std::move(channel));
    }

    /// A push channel that connects to address, on the given transport.
    auto Peer(string const& transport, test::IpcAddress const& address) -> unique_ptr<Channel>
    {
        auto& factory = fPeerFactories[transport];
        if (!factory) {
            factory = TransportFactory::CreateTransportFactory(transport, tools::Uuid(), &fPeerConfig);
        }
        // Unique names: zeromq monitors each socket on an inproc address made of the name.
        auto channel = make_unique<Channel>("peer" + to_string(fPeerCount++), "push", factory);
        EXPECT_TRUE(channel->Connect(address));
        return channel;
    }

    static void Send(Channel& channel, string const& text)
    {
        auto msg(channel.NewSimpleMessage(text));
        ASSERT_EQ(channel.Send(msg, 5000), static_cast<int64_t>(text.size()));
    }

    /// Wait until Run() ended by itself, and the device is Ready again.
    static auto RunEnds(Device& device) -> bool
    {
        return test::WaitUntil([&] { return device.GetCurrentState() == State::Ready; });
    }

    ProgOptions fConfig;
    string fSession;
    ProgOptions fPeerConfig;
    map<string, shared_ptr<TransportFactory>> fPeerFactories;
    int fPeerCount = 0;
};

TEST_P(DataCallbacks, SingleMessage)
{
    Device device;
    device.SetConfig(fConfig);
    test::IpcAddress const address("ondata_single");
    AddInput(device, "data", address);

    vector<string> received;
    device.OnData("data", [&](MessagePtr& msg, int index) {
        EXPECT_EQ(index, 0);
        received.push_back(AsString(*msg));
        return received.size() < 3; // false ends Run()
    });

    test::StateMachineThread fsm(device);
    test::Control(device, test::Cycle::ToRun);
    auto peer = Peer(GetParam().transport, address);
    for (auto const& text : {"1", "2", "3"}) {
        Send(*peer, text);
    }

    ASSERT_TRUE(RunEnds(device));
    EXPECT_EQ(received, (vector<string>{"1", "2", "3"}));
}

TEST_P(DataCallbacks, Multipart)
{
    Device device;
    device.SetConfig(fConfig);
    test::IpcAddress const address("ondata_multipart");
    AddInput(device, "data", address);

    vector<string> received;
    device.OnData("data", [&](Parts& parts, int) {
        string joined;
        for (auto const& part : parts) {
            joined += AsString(*part) + "|";
        }
        received.push_back(joined);
        return false;
    });

    test::StateMachineThread fsm(device);
    test::Control(device, test::Cycle::ToRun);
    auto peer = Peer(GetParam().transport, address);
    Parts parts(peer->NewSimpleMessage("a"), peer->NewSimpleMessage("bc"));
    ASSERT_EQ(peer->Send(parts, 5000), 3);

    ASSERT_TRUE(RunEnds(device));
    EXPECT_EQ(received, (vector<string>{"a|bc|"}));
}

TEST_P(DataCallbacks, TwoChannels)
{
    Device device;
    device.SetConfig(fConfig);
    test::IpcAddress const address1("ondata_two_1");
    test::IpcAddress const address2("ondata_two_2");
    AddInput(device, "one", address1);
    AddInput(device, "two", address2);

    mutex mtx;
    set<string> received;
    auto record = [&](string const& text) {
        lock_guard<mutex> lock(mtx);
        received.insert(text);
        return received.size() < 2;
    };
    device.OnData("one", [&](MessagePtr& msg, int) { return record("one:" + AsString(*msg)); });
    device.OnData("two", [&](Parts& parts, int) { return record("two:" + AsString(*parts.At(0))); });

    test::StateMachineThread fsm(device);
    test::Control(device, test::Cycle::ToRun);
    auto peer1 = Peer(GetParam().transport, address1);
    auto peer2 = Peer(GetParam().transport, address2);
    Send(*peer1, "x");
    Parts parts(peer2->NewSimpleMessage("y"));
    ASSERT_EQ(peer2->Send(parts, 5000), 1);

    ASSERT_TRUE(RunEnds(device));
    lock_guard<mutex> lock(mtx);
    EXPECT_EQ(received, (set<string>{"one:x", "two:y"}));
}

INSTANTIATE_TEST_SUITE_P(, DataCallbacks, test::AllTransports(), test::TransportParamName());

// One input per transport: the device polls each transport in its own thread.
class MultipleTransports : public DataCallbacks
{};

TEST_P(MultipleTransports, OneInputEach)
{
    Device device;
    device.SetConfig(fConfig);
    test::IpcAddress const zmqAddress("ondata_zmq");
    test::IpcAddress const shmAddress("ondata_shm");
    AddInput(device, "zmq", zmqAddress, "zeromq");
    AddInput(device, "shm", shmAddress, "shmem");

    mutex mtx;
    set<string> received;
    auto record = [&](string const& text) {
        lock_guard<mutex> lock(mtx);
        received.insert(text);
        return received.size() < 2;
    };
    device.OnData("zmq", [&](MessagePtr& msg, int) { return record("zmq:" + AsString(*msg)); });
    device.OnData("shm", [&](MessagePtr& msg, int) { return record("shm:" + AsString(*msg)); });

    test::StateMachineThread fsm(device);
    test::Control(device, test::Cycle::ToRun);
    EXPECT_EQ(device.GetChannel("zmq").GetTransportType(), Transport::ZMQ);
    EXPECT_EQ(device.GetChannel("shm").GetTransportType(), Transport::SHM);
    auto zmqPeer = Peer("zeromq", zmqAddress);
    auto shmPeer = Peer("shmem", shmAddress);
    Send(*zmqPeer, "1");
    Send(*shmPeer, "2");

    ASSERT_TRUE(RunEnds(device));
    lock_guard<mutex> lock(mtx);
    EXPECT_EQ(received, (set<string>{"zmq:1", "shm:2"}));
}

// The default transport of the device does not matter for this test.
INSTANTIATE_TEST_SUITE_P(, MultipleTransports, test::BasicTransports(), test::TransportParamName());

class Conditional : public Device
{
  public:
    explicit Conditional(int iterations)
        : fIterations(iterations)
    {}

    std::atomic<int> fCalls{0};

  protected:
    bool ConditionalRun() override { return ++fCalls < fIterations; }

  private:
    int fIterations;
};

TEST(ConditionalRun, EndsRunWhenItReturnsFalse)
{
    ProgOptions config;
    test::Configure(config);
    Conditional device(5);
    device.SetConfig(config);

    test::StateMachineThread fsm(device);
    test::Control(device, test::Cycle::ToRun);

    ASSERT_TRUE(test::WaitUntil([&] { return device.GetCurrentState() == State::Ready; }));
    EXPECT_EQ(device.fCalls, 5);
}

TEST(ConditionalRun, Rate)
{
    ProgOptions config;
    test::Configure(config);
    config.SetProperty<float>("rate", 100.F);
    Conditional device(21);
    device.SetConfig(config);

    test::StateMachineThread fsm(device);
    auto const start = chrono::steady_clock::now();
    test::Control(device, test::Cycle::ToRun);
    ASSERT_TRUE(test::WaitUntil([&] { return device.GetCurrentState() == State::Ready; }));

    // 20 sleeps at 100 Hz take approx. 200 ms; check only a loose lower bound.
    EXPECT_GE(chrono::steady_clock::now() - start, chrono::milliseconds(140));
    EXPECT_EQ(device.fCalls, 21);
}

} // namespace

/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Fixtures.h"

#include <asio.hpp>
#include <dds/dds.h>
#include <fairmq/sdk/Topology.h>
#include <fairmq/Tools.h>

#include <thread>

namespace {

using Topology = fair::mq::test::TopologyFixture;
using MultipleTopologies = fair::mq::test::MultipleTopologiesFixture;

void control(fair::mq::sdk::Topology& topo)
{
    using fair::mq::sdk::TopologyTransition;

    for (auto transition : {TopologyTransition::InitDevice,
                            TopologyTransition::CompleteInit,
                            TopologyTransition::Bind,
                            TopologyTransition::Connect,
                            TopologyTransition::InitTask,
                            TopologyTransition::Run,
                            TopologyTransition::Stop,
                            TopologyTransition::ResetTask,
                            TopologyTransition::ResetDevice,
                            TopologyTransition::End}) {
        ASSERT_EQ(topo.ChangeState(transition).first, std::error_code());
    }
}

TEST(TopologyHelper, MakeTopology)
{
    using namespace fair::mq;

    // This is only needed for this unit test
    test::LoggerConfig cfg;
    sdk::DDSEnv env(CMAKE_CURRENT_BINARY_DIR);

    std::string topoFile(tools::ToString(SDK_TESTSUITE_SOURCE_DIR, "/test_topo.xml"));
    dds::topology_api::CTopology nativeTopo(topoFile);
    auto nativeSession(std::make_shared<dds::tools_api::CSession>());
    nativeSession->create();
    EXPECT_THROW(sdk::MakeTopology(nativeTopo, nativeSession, env), sdk::RuntimeError);
    nativeSession->shutdown();
}

TEST_F(MultipleTopologies, Construction)
{
    using namespace fair::mq;

    std::array<sdk::Topology, mNumSessions> topos{
        sdk::Topology(mDDSTopologies[0], mDDSSessions[0]),
        sdk::Topology(mDDSTopologies[1], mDDSSessions[1])
    };
}

TEST_F(MultipleTopologies, ChangeStateFullDeviceLifecycle)
{
    using namespace fair::mq;

    std::array<sdk::Topology, mNumSessions> topos{
        sdk::Topology(mDDSTopologies[0], mDDSSessions[0]),
        sdk::Topology(mDDSTopologies[1], mDDSSessions[1])
    };

    for (int i = 0; i < mNumSessions; ++i) {
        using fair::mq::sdk::TopologyTransition;

        for (auto transition : {TopologyTransition::InitDevice,
                                TopologyTransition::CompleteInit,
                                TopologyTransition::Bind,
                                TopologyTransition::Connect,
                                TopologyTransition::InitTask,
                                TopologyTransition::Run,
                                TopologyTransition::Stop,
                                TopologyTransition::ResetTask,
                                TopologyTransition::ResetDevice,
                                TopologyTransition::End}) {
            ASSERT_EQ(topos[i].ChangeState(transition).first, std::error_code());
        }
    }
}

TEST_F(MultipleTopologies, ChangeStateFullDeviceLifecycleConcurrent)
{
    using namespace fair::mq;

    std::array<sdk::Topology, mNumSessions> topos{
        sdk::Topology(mDDSTopologies[0], mDDSSessions[0]),
        sdk::Topology(mDDSTopologies[1], mDDSSessions[1])
    };

    std::thread t0(control, std::ref(topos[0]));
    std::thread t1(control, std::ref(topos[1]));
    t0.join();
    t1.join();
}


TEST_F(Topology, Construction)
{
    fair::mq::sdk::Topology topo(mDDSTopo, mDDSSession);
}

TEST_F(Topology, Construction2)
{
    fair::mq::sdk::Topology topo(mIoContext.get_executor(), mDDSTopo, mDDSSession);
}

TEST_F(Topology, AsyncChangeState)
{
    using namespace fair::mq;

    tools::SharedSemaphore blocker;
    sdk::Topology topo(mDDSTopo, mDDSSession);
    topo.AsyncChangeState(
        sdk::TopologyTransition::InitDevice,
        [=](std::error_code ec, sdk::TopologyState) mutable {
            LOG(info) << ec;
            EXPECT_EQ(ec, std::error_code());
            blocker.Signal();
        });
    blocker.Wait();
}

TEST_F(Topology, AsyncChangeStateWithCustomExecutor)
{
    using namespace fair::mq;

    sdk::Topology topo(mIoContext.get_executor(), mDDSTopo, mDDSSession);
    topo.AsyncChangeState(
        sdk::TopologyTransition::InitDevice,
        [](std::error_code ec, sdk::TopologyState) {
            LOG(info) << ec;
            EXPECT_EQ(ec, std::error_code());
        });

    mIoContext.run();
}

TEST_F(Topology, AsyncChangeStateFuture)
{
    using namespace fair::mq;

    sdk::Topology topo(mIoContext.get_executor(), mDDSTopo, mDDSSession);
    auto fut(topo.AsyncChangeState(
        sdk::TopologyTransition::InitDevice,
        asio::use_future));
    std::thread t([&]() { mIoContext.run(); });
    bool success(false);

    try {
        sdk::TopologyState state = fut.get();
        success = true;
    } catch (const std::system_error& ex) {
        LOG(error) << ex.what();
    }

    EXPECT_TRUE(success);
    t.join();
}

#if defined(ASIO_HAS_CO_AWAIT)
TEST_F(Topology, AsyncChangeStateCoroutine)
{
    using namespace fair::mq;

    bool success(false);
    asio::co_spawn(
        mIoContext.get_executor(),
        [&]() mutable -> asio::awaitable<void> {
            auto executor = co_await asio::this_coro::executor;
            sdk::Topology topo(executor, mDDSTopo, mDDSSession);
            try {
                sdk::TopologyState state = co_await topo.AsyncChangeState(
                    sdk::TopologyTransition::InitDevice, asio::use_awaitable);
                success = true;
            } catch (const std::system_error& ex) {
                LOG(error) << ex.what();
            }
        },
        asio::detached);

    mIoContext.run();
    EXPECT_TRUE(success);
}
#endif

TEST_F(Topology, ChangeState)
{
    using namespace fair::mq;

    sdk::Topology topo(mDDSTopo, mDDSSession);
    auto result(topo.ChangeState(sdk::TopologyTransition::InitDevice));
    LOG(info) << result.first;

    EXPECT_EQ(result.first, std::error_code());
    EXPECT_NO_THROW(sdk::AggregateState(result.second));
    EXPECT_EQ(sdk::StateEqualsTo(result.second, sdk::DeviceState::InitializingDevice), true);
    auto const currentState = topo.GetCurrentState();
    EXPECT_NO_THROW(sdk::AggregateState(currentState));
    EXPECT_EQ(sdk::StateEqualsTo(currentState, sdk::DeviceState::InitializingDevice), true);
}

TEST_F(Topology, MixedState)
{
    using namespace fair::mq;

    sdk::Topology topo(mDDSTopo, mDDSSession);
    auto result1(topo.ChangeState(sdk::TopologyTransition::InitDevice, "main/Sampler.*"));
    LOG(info) << result1.first;

    EXPECT_EQ(result1.first, std::error_code());
    EXPECT_EQ(sdk::AggregateState(result1.second), sdk::AggregatedTopologyState::Mixed);
    EXPECT_EQ(sdk::StateEqualsTo(result1.second, sdk::DeviceState::InitializingDevice), false);
    auto const currentState1 = topo.GetCurrentState();
    EXPECT_EQ(sdk::AggregateState(currentState1), sdk::AggregatedTopologyState::Mixed);
    EXPECT_EQ(sdk::StateEqualsTo(currentState1, sdk::DeviceState::InitializingDevice), false);

    auto result2(topo.ChangeState(sdk::TopologyTransition::InitDevice, "main/SinkGroup/.*"));
    LOG(info) << result2.first;

    EXPECT_EQ(result2.first, std::error_code());
    EXPECT_EQ(sdk::AggregateState(result2.second), sdk::AggregatedTopologyState::InitializingDevice);
    EXPECT_EQ(sdk::StateEqualsTo(result2.second, sdk::DeviceState::InitializingDevice), true);
    auto const currentState2 = topo.GetCurrentState();
    EXPECT_EQ(sdk::AggregateState(currentState2), sdk::AggregatedTopologyState::InitializingDevice);
    EXPECT_EQ(sdk::StateEqualsTo(currentState2, sdk::DeviceState::InitializingDevice), true);
}

TEST_F(Topology, AsyncChangeStateConcurrent)
{
    using namespace fair::mq;

    sdk::Topology topo(mDDSTopo, mDDSSession);
    topo.AsyncChangeState(sdk::TopologyTransition::InitDevice, "main/Sampler.*",
                          [](std::error_code ec, sdk::TopologyState) mutable {
                              LOG(info) << "ChangeState for Sampler: " << ec;
                              EXPECT_EQ(ec, std::error_code());
                          });
    topo.AsyncChangeState(sdk::TopologyTransition::InitDevice, "main/SinkGroup/.*",
                          [](std::error_code ec, sdk::TopologyState) mutable {
                              LOG(info) << "ChangeState for Sinks: " << ec;
                              EXPECT_EQ(ec, std::error_code());
                          });

    topo.WaitForState(sdk::DeviceState::InitializingDevice);
    auto const currentState = topo.GetCurrentState();
    EXPECT_NO_THROW(sdk::AggregateState(currentState));
    EXPECT_EQ(sdk::StateEqualsTo(currentState, sdk::DeviceState::InitializingDevice), true);
}

TEST_F(Topology, AsyncChangeStateTimeout)
{
    using namespace fair::mq;

    sdk::Topology topo(mIoContext.get_executor(), mDDSTopo, mDDSSession);
    topo.AsyncChangeState(sdk::TopologyTransition::InitDevice,
                          std::chrono::milliseconds(1),
                          [](std::error_code ec, sdk::TopologyState) {
                              LOG(info) << ec;
                              EXPECT_EQ(ec, MakeErrorCode(ErrorCode::OperationTimeout));
                          });

    mIoContext.run();
}

TEST_F(Topology, AsyncChangeStateCollectionView)
{
    using namespace fair::mq;

    tools::SharedSemaphore blocker;
    sdk::Topology topo(mDDSTopo, mDDSSession);
    topo.AsyncChangeState(
        sdk::TopologyTransition::InitDevice,
        [=](std::error_code ec, sdk::TopologyState state) mutable {
            LOG(info) << ec;
            sdk::TopologyStateByCollection cstate(sdk::GroupByCollectionId(state));
            LOG(debug) << "num collections: " << cstate.size();
            ASSERT_EQ(cstate.size(), 5);
            for (const auto& c : cstate) {
                LOG(debug) << "\t" << c.first;
                sdk::AggregatedTopologyState s;
                ASSERT_NO_THROW(s = sdk::AggregateState(c.second));
                ASSERT_EQ(s, static_cast<sdk::AggregatedTopologyState>(State::InitializingDevice));
                LOG(debug) << "\tAggregated state: " << s;
                for (const auto& ds : c.second) {
                    LOG(debug) << "\t\t" << ds.state;
                }
            }
            EXPECT_EQ(ec, std::error_code());
            blocker.Signal();
        });
    blocker.Wait();
}

TEST_F(Topology, ChangeStateFullDeviceLifecycle)
{
    using namespace fair::mq;
    using fair::mq::sdk::TopologyTransition;

    sdk::Topology topo(mDDSTopo, mDDSSession);
    for (auto transition : {TopologyTransition::InitDevice,
                            TopologyTransition::CompleteInit,
                            TopologyTransition::Bind,
                            TopologyTransition::Connect,
                            TopologyTransition::InitTask,
                            TopologyTransition::Run,
                            TopologyTransition::Stop,
                            TopologyTransition::ResetTask,
                            TopologyTransition::ResetDevice,
                            TopologyTransition::End}) {
        ASSERT_EQ(topo.ChangeState(transition).first, std::error_code());
    }
}

TEST_F(Topology, WaitForStateFullDeviceLifecycle)
{
    using namespace fair::mq;
    using fair::mq::sdk::TopologyTransition;

    sdk::Topology topo(mDDSTopo, mDDSSession);
    topo.AsyncWaitForState(sdk::DeviceState::ResettingDevice, [](std::error_code ec){
        ASSERT_EQ(ec, std::error_code());
    });
    for (auto transition : {TopologyTransition::InitDevice,
                            TopologyTransition::CompleteInit,
                            TopologyTransition::Bind,
                            TopologyTransition::Connect,
                            TopologyTransition::InitTask,
                            TopologyTransition::Run,
                            TopologyTransition::Stop,
                            TopologyTransition::ResetTask,
                            TopologyTransition::ResetDevice,
                            TopologyTransition::End}) {
        topo.ChangeState(transition);
        ASSERT_EQ(topo.WaitForState(sdk::expectedState.at(transition)), std::error_code());
    }
}

TEST_F(Topology, ChangeStateFullDeviceLifecycle2)
{
    using namespace fair::mq;
    using fair::mq::sdk::TopologyTransition;

    sdk::Topology topo(mDDSTopo, mDDSSession);
    for (int i(0); i < 10; ++i) {
        for (auto transition : {TopologyTransition::InitDevice,
                                TopologyTransition::CompleteInit,
                                TopologyTransition::Bind,
                                TopologyTransition::Connect,
                                TopologyTransition::InitTask,
                                TopologyTransition::Run}) {
            ASSERT_EQ(topo.ChangeState(transition).first, std::error_code());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for (auto transition : {TopologyTransition::Stop,
                                TopologyTransition::ResetTask,
                                TopologyTransition::ResetDevice}) {
            ASSERT_EQ(topo.ChangeState(transition).first, std::error_code());
        }
    }
}

TEST_F(Topology, SetProperties)
{
    using namespace fair::mq;
    using fair::mq::sdk::TopologyTransition;

    sdk::Topology topo(mDDSTopo, mDDSSession);
    ASSERT_EQ(topo.ChangeState(TopologyTransition::InitDevice).first, std::error_code());

    auto const result1 = topo.SetProperties({{"key1", "val1"}});
    LOG(info) << result1.first;
    ASSERT_EQ(result1.first, std::error_code());
    ASSERT_EQ(result1.second.size(), 0);
    auto const result2 = topo.SetProperties({{"key2", "val2"}, {"key3", "val3"}});
    LOG(info) << result2.first;
    ASSERT_EQ(result2.first, std::error_code());
    ASSERT_EQ(result2.second.size(), 0);

    ASSERT_EQ(topo.ChangeState(TopologyTransition::CompleteInit).first, std::error_code());
    ASSERT_EQ(topo.ChangeState(TopologyTransition::ResetDevice).first, std::error_code());
}

TEST_F(Topology, AsyncSetPropertiesConcurrent)
{
    using namespace fair::mq;
    using fair::mq::sdk::TopologyTransition;

    sdk::Topology topo(mDDSTopo, mDDSSession);
    ASSERT_EQ(topo.ChangeState(TopologyTransition::InitDevice).first, std::error_code());

    tools::SharedSemaphore blocker(2);
    topo.AsyncSetProperties(
        {{"key1", "val1"}},
        [=](std::error_code ec, sdk::FailedDevices failed) mutable {
            LOG(info) << ec;
            ASSERT_EQ(ec, std::error_code());
            ASSERT_EQ(failed.size(), 0);
            blocker.Signal();
        });
    topo.AsyncSetProperties(
        {{"key2", "val2"}, {"key3", "val3"}},
        [=](std::error_code ec, sdk::FailedDevices failed) mutable {
            LOG(info) << ec;
            ASSERT_EQ(ec, std::error_code());
            ASSERT_EQ(failed.size(), 0);
            blocker.Signal();
        });
    blocker.Wait();

    ASSERT_EQ(topo.ChangeState(TopologyTransition::CompleteInit).first, std::error_code());
    ASSERT_EQ(topo.ChangeState(TopologyTransition::ResetDevice).first, std::error_code());
}

TEST_F(Topology, AsyncSetPropertiesTimeout)
{
    using namespace fair::mq;
    using fair::mq::sdk::TopologyTransition;

    sdk::Topology topo(mDDSTopo, mDDSSession);
    ASSERT_EQ(topo.ChangeState(TopologyTransition::InitDevice).first, std::error_code());

    topo.AsyncSetProperties({{"key1", "val1"}},
                            "",
                            std::chrono::microseconds(1),
                            [=](std::error_code ec, sdk::FailedDevices) mutable {
                                LOG(info) << ec;
                                EXPECT_EQ(ec, MakeErrorCode(ErrorCode::OperationTimeout));
                            });

    ASSERT_EQ(topo.ChangeState(TopologyTransition::CompleteInit).first, std::error_code());
    ASSERT_EQ(topo.ChangeState(TopologyTransition::ResetDevice).first, std::error_code());
}

TEST_F(Topology, SetPropertiesMixed)
{
    using namespace fair::mq;
    using fair::mq::sdk::TopologyTransition;

    sdk::Topology topo(mDDSTopo, mDDSSession);
    ASSERT_EQ(topo.ChangeState(TopologyTransition::InitDevice).first, std::error_code());

    tools::SharedSemaphore blocker;
    topo.AsyncSetProperties(
        {{"key1", "val1"}},
        [=](std::error_code ec, sdk::FailedDevices failed) mutable {
            LOG(info) << ec;
            ASSERT_EQ(ec, std::error_code());
            ASSERT_EQ(failed.size(), 0);
            blocker.Signal();
        });

    auto result = topo.SetProperties({{"key2", "val2"}});
    LOG(info) << result.first;
    ASSERT_EQ(result.first, std::error_code());
    ASSERT_EQ(result.second.size(), 0);

    blocker.Wait();

    ASSERT_EQ(topo.ChangeState(TopologyTransition::CompleteInit).first, std::error_code());
    ASSERT_EQ(topo.ChangeState(TopologyTransition::ResetDevice).first, std::error_code());
}

TEST_F(Topology, GetProperties)
{
    using namespace fair::mq;
    using fair::mq::sdk::TopologyTransition;

    sdk::Topology topo(mDDSTopo, mDDSSession);
    ASSERT_EQ(topo.ChangeState(TopologyTransition::InitDevice).first, std::error_code());

    auto const result = topo.GetProperties("^(session|id)$");
    LOG(info) << result.first;
    ASSERT_EQ(result.first, std::error_code());
    ASSERT_EQ(result.second.failed.size(), 0);
    for (auto const& d : result.second.devices) {
        LOG(info) << d.first;
        ASSERT_EQ(d.second.props.size(), 2);
        for (auto const& p : d.second.props) {
            LOG(info) << p.first << " : " << p.second;
        }
    }

    ASSERT_EQ(topo.ChangeState(TopologyTransition::CompleteInit).first, std::error_code());
    ASSERT_EQ(topo.ChangeState(TopologyTransition::ResetDevice).first, std::error_code());
}

TEST_F(Topology, SetAndGetProperties)
{
    using namespace fair::mq;
    using fair::mq::sdk::TopologyTransition;

    sdk::Topology topo(mDDSTopo, mDDSSession);
    ASSERT_EQ(topo.ChangeState(TopologyTransition::InitDevice).first, std::error_code());

    sdk::DeviceProperties const props{{"key1", "val1"}, {"key2", "val2"}};

    auto const result1 = topo.SetProperties(props);
    LOG(info) << result1.first;
    ASSERT_EQ(result1.first, std::error_code());
    ASSERT_EQ(result1.second.size(), 0);

    auto const result2 = topo.GetProperties("^key.*");
    LOG(info) << result2.first;
    ASSERT_EQ(result2.first, std::error_code());
    ASSERT_EQ(result2.second.failed.size(), 0);
    ASSERT_EQ(result2.second.devices.size(), 6);
    for (auto const& d : result2.second.devices) {
        ASSERT_EQ(d.second.props, props);
    }

    ASSERT_EQ(topo.ChangeState(TopologyTransition::CompleteInit).first, std::error_code());
    ASSERT_EQ(topo.ChangeState(TopologyTransition::ResetDevice).first, std::error_code());
}

TEST(Topology2, AggregatedTopologyStateComparison)
{
    using namespace fair::mq::sdk;
    ASSERT_TRUE(DeviceState::Undefined == AggregatedTopologyState::Undefined);
    ASSERT_TRUE(AggregatedTopologyState::Undefined == DeviceState::Undefined);
    ASSERT_TRUE(DeviceState::Ok == AggregatedTopologyState::Ok);
    ASSERT_TRUE(DeviceState::Error == AggregatedTopologyState::Error);
    ASSERT_TRUE(DeviceState::Idle == AggregatedTopologyState::Idle);
    ASSERT_TRUE(DeviceState::InitializingDevice == AggregatedTopologyState::InitializingDevice);
    ASSERT_TRUE(DeviceState::Initialized == AggregatedTopologyState::Initialized);
    ASSERT_TRUE(DeviceState::Binding == AggregatedTopologyState::Binding);
    ASSERT_TRUE(DeviceState::Bound == AggregatedTopologyState::Bound);
    ASSERT_TRUE(DeviceState::Connecting == AggregatedTopologyState::Connecting);
    ASSERT_TRUE(DeviceState::DeviceReady == AggregatedTopologyState::DeviceReady);
    ASSERT_TRUE(DeviceState::InitializingTask == AggregatedTopologyState::InitializingTask);
    ASSERT_TRUE(DeviceState::Ready == AggregatedTopologyState::Ready);
    ASSERT_TRUE(DeviceState::Running == AggregatedTopologyState::Running);
    ASSERT_TRUE(DeviceState::ResettingTask == AggregatedTopologyState::ResettingTask);
    ASSERT_TRUE(DeviceState::ResettingDevice == AggregatedTopologyState::ResettingDevice);
    ASSERT_TRUE(DeviceState::Exiting == AggregatedTopologyState::Exiting);

    ASSERT_TRUE(GetAggregatedTopologyState("UNDEFINED") == AggregatedTopologyState::Undefined);
    ASSERT_TRUE(GetAggregatedTopologyState("OK") == AggregatedTopologyState::Ok);
    ASSERT_TRUE(GetAggregatedTopologyState("ERROR") == AggregatedTopologyState::Error);
    ASSERT_TRUE(GetAggregatedTopologyState("IDLE") == AggregatedTopologyState::Idle);
    ASSERT_TRUE(GetAggregatedTopologyState("INITIALIZING DEVICE") == AggregatedTopologyState::InitializingDevice);
    ASSERT_TRUE(GetAggregatedTopologyState("INITIALIZED") == AggregatedTopologyState::Initialized);
    ASSERT_TRUE(GetAggregatedTopologyState("BINDING") == AggregatedTopologyState::Binding);
    ASSERT_TRUE(GetAggregatedTopologyState("BOUND") == AggregatedTopologyState::Bound);
    ASSERT_TRUE(GetAggregatedTopologyState("CONNECTING") == AggregatedTopologyState::Connecting);
    ASSERT_TRUE(GetAggregatedTopologyState("DEVICE READY") == AggregatedTopologyState::DeviceReady);
    ASSERT_TRUE(GetAggregatedTopologyState("INITIALIZING TASK") == AggregatedTopologyState::InitializingTask);
    ASSERT_TRUE(GetAggregatedTopologyState("READY") == AggregatedTopologyState::Ready);
    ASSERT_TRUE(GetAggregatedTopologyState("RUNNING") == AggregatedTopologyState::Running);
    ASSERT_TRUE(GetAggregatedTopologyState("RESETTING TASK") == AggregatedTopologyState::ResettingTask);
    ASSERT_TRUE(GetAggregatedTopologyState("RESETTING DEVICE") == AggregatedTopologyState::ResettingDevice);
    ASSERT_TRUE(GetAggregatedTopologyState("EXITING") == AggregatedTopologyState::Exiting);
    ASSERT_TRUE(GetAggregatedTopologyState("MIXED") == AggregatedTopologyState::Mixed);

    ASSERT_TRUE("UNDEFINED" == GetAggregatedTopologyStateName(AggregatedTopologyState::Undefined));
    ASSERT_TRUE("OK" == GetAggregatedTopologyStateName(AggregatedTopologyState::Ok));
    ASSERT_TRUE("ERROR" == GetAggregatedTopologyStateName(AggregatedTopologyState::Error));
    ASSERT_TRUE("IDLE" == GetAggregatedTopologyStateName(AggregatedTopologyState::Idle));
    ASSERT_TRUE("INITIALIZING DEVICE" == GetAggregatedTopologyStateName(AggregatedTopologyState::InitializingDevice));
    ASSERT_TRUE("INITIALIZED" == GetAggregatedTopologyStateName(AggregatedTopologyState::Initialized));
    ASSERT_TRUE("BINDING" == GetAggregatedTopologyStateName(AggregatedTopologyState::Binding));
    ASSERT_TRUE("BOUND" == GetAggregatedTopologyStateName(AggregatedTopologyState::Bound));
    ASSERT_TRUE("CONNECTING" == GetAggregatedTopologyStateName(AggregatedTopologyState::Connecting));
    ASSERT_TRUE("DEVICE READY" == GetAggregatedTopologyStateName(AggregatedTopologyState::DeviceReady));
    ASSERT_TRUE("INITIALIZING TASK" == GetAggregatedTopologyStateName(AggregatedTopologyState::InitializingTask));
    ASSERT_TRUE("READY" == GetAggregatedTopologyStateName(AggregatedTopologyState::Ready));
    ASSERT_TRUE("RUNNING" == GetAggregatedTopologyStateName(AggregatedTopologyState::Running));
    ASSERT_TRUE("RESETTING TASK" == GetAggregatedTopologyStateName(AggregatedTopologyState::ResettingTask));
    ASSERT_TRUE("RESETTING DEVICE" == GetAggregatedTopologyStateName(AggregatedTopologyState::ResettingDevice));
    ASSERT_TRUE("EXITING" == GetAggregatedTopologyStateName(AggregatedTopologyState::Exiting));
    ASSERT_TRUE("MIXED" == GetAggregatedTopologyStateName(AggregatedTopologyState::Mixed));
}

}   // namespace

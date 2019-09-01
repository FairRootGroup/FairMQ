/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Fixtures.h"

#include <asio.hpp>
#include <DDS/Topology.h>
#include <DDS/Tools.h>
#include <fairmq/sdk/Topology.h>
#include <fairmq/Tools.h>

namespace {

using Topology = fair::mq::test::TopologyFixture;

TEST(TopologyHelper, MakeTopology)
{
    using namespace fair::mq;

    // This is only needed for this unit test
    test::LoggerConfig cfg;
    sdk::DDSEnv env(CMAKE_CURRENT_BINARY_DIR);
    /////////////////////////////////////

    dds::topology_api::CTopology nativeTopo(
        tools::ToString(SDK_TESTSUITE_SOURCE_DIR, "/test_topo.xml"));
    auto nativeSession(std::make_shared<dds::tools_api::CSession>());
    nativeSession->create();
    EXPECT_THROW(sdk::MakeTopology(nativeTopo, nativeSession, env), sdk::RuntimeError);
    nativeSession->shutdown();
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
}

TEST_F(Topology, AsyncChangeStateConcurrent)
{
    using namespace fair::mq;

    sdk::Topology topo(mDDSTopo, mDDSSession);
    tools::SharedSemaphore blocker;
    topo.AsyncChangeState(sdk::TopologyTransition::InitDevice,
                          [blocker](std::error_code ec, sdk::TopologyState) mutable {
                              LOG(info) << "result for valid ChangeState: " << ec;
                              blocker.Signal();
                          });
    topo.AsyncChangeState(sdk::TopologyTransition::Stop,
                          [](std::error_code ec, sdk::TopologyState) {
                              LOG(ERROR) << "Expected error: " << ec;
                              EXPECT_EQ(ec, MakeErrorCode(ErrorCode::OperationInProgress));
                          });
    blocker.Wait();
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

}   // namespace

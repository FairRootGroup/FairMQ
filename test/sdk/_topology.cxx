/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Fixtures.h"

#include <DDS/Topology.h>
#include <DDS/Tools.h>
#include <fairmq/sdk/Topology.h>
#include <fairmq/Tools.h>

namespace {

using Topology = fair::mq::test::TopologyFixture;

TEST(Topology2, ConstructionWithNativeDdsApiObjects)
{
    // This is only needed for this unit test
    fair::mq::test::LoggerConfig cfg;
    fair::mq::sdk::DDSEnv env(CMAKE_CURRENT_BINARY_DIR);
    /////////////////////////////////////////

    // Example usage:
    dds::topology_api::CTopology nativeTopo(
        fair::mq::tools::ToString(SDK_TESTSUITE_SOURCE_DIR, "/test_topo.xml"));
    auto nativeSession(std::make_shared<dds::tools_api::CSession>());
    nativeSession->create();
    EXPECT_THROW(fair::mq::sdk::Topology topo(nativeTopo, nativeSession, env), std::runtime_error);
}

TEST_F(Topology, Construction)
{
    fair::mq::sdk::Topology topo(mDDSTopo, mDDSSession);
}

TEST_F(Topology, ChangeStateAsync)
{
    using fair::mq::sdk::Topology;
    using fair::mq::sdk::TopologyTransition;

    Topology topo(mDDSTopo, mDDSSession);
    fair::mq::tools::Semaphore blocker;
    topo.ChangeState(
        TopologyTransition::InitDevice, [&blocker, &topo](Topology::ChangeStateResult result) {
            LOG(info) << result;
            EXPECT_EQ(result.rc, fair::mq::AsyncOpResultCode::Ok);
            EXPECT_NO_THROW(fair::mq::sdk::AggregateState(result.state));
            EXPECT_EQ(
                fair::mq::sdk::StateEqualsTo(result.state, fair::mq::sdk::DeviceState::InitializingDevice),
                true);
            blocker.Signal();
        });
    blocker.Wait();
}

TEST_F(Topology, ChangeStateSync)
{
    using fair::mq::sdk::Topology;
    using fair::mq::sdk::TopologyTransition;

    Topology topo(mDDSTopo, mDDSSession);
    auto result(topo.ChangeState(TopologyTransition::InitDevice));

    EXPECT_EQ(result.rc, fair::mq::AsyncOpResultCode::Ok);
    EXPECT_NO_THROW(fair::mq::sdk::AggregateState(result.state));
    EXPECT_EQ(
        fair::mq::sdk::StateEqualsTo(result.state, fair::mq::sdk::DeviceState::InitializingDevice),
        true);
}

TEST_F(Topology, ChangeStateConcurrent)
{
    using fair::mq::sdk::Topology;
    using fair::mq::sdk::TopologyTransition;

    Topology topo(mDDSTopo, mDDSSession);
    fair::mq::tools::Semaphore blocker;
    topo.ChangeState(TopologyTransition::InitDevice,
                     [&blocker](Topology::ChangeStateResult result) {
                         LOG(info) << "result for valid ChangeState: " << result;
                         blocker.Signal();
                     });
    EXPECT_THROW(topo.ChangeState(TopologyTransition::Stop,
                                  [&blocker](Topology::ChangeStateResult) {}),
                 std::runtime_error);
    blocker.Wait();
}

TEST_F(Topology, ChangeStateTimeout)
{
    using fair::mq::sdk::Topology;
    using fair::mq::sdk::TopologyTransition;

    Topology topo(mDDSTopo, mDDSSession);
    fair::mq::tools::Semaphore blocker;
    topo.ChangeState(TopologyTransition::InitDevice, [&](Topology::ChangeStateResult result) {
        LOG(info) << result;
        EXPECT_EQ(result.rc, fair::mq::AsyncOpResultCode::Timeout);
        blocker.Signal();
    }, std::chrono::milliseconds(1));
    blocker.Wait();
}

TEST_F(Topology, ChangeStateFullDeviceLifecycle)
{
    using fair::mq::sdk::Topology;
    using fair::mq::sdk::TopologyTransition;

    Topology topo(mDDSTopo, mDDSSession);
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
        ASSERT_EQ(topo.ChangeState(transition).rc, fair::mq::AsyncOpResultCode::Ok);
    }
}

TEST_F(Topology, ChangeStateFullDeviceLifecycle2)
{
    using fair::mq::sdk::Topology;
    using fair::mq::sdk::TopologyTransition;

    Topology topo(mDDSTopo, mDDSSession);
    for (int i(0); i < 10; ++i) {
        for (auto transition : {TopologyTransition::InitDevice,
                                TopologyTransition::CompleteInit,
                                TopologyTransition::Bind,
                                TopologyTransition::Connect,
                                TopologyTransition::InitTask,
                                TopologyTransition::Run}) {
            ASSERT_EQ(topo.ChangeState(transition).rc, fair::mq::AsyncOpResultCode::Ok);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for (auto transition : {TopologyTransition::Stop,
                                TopologyTransition::ResetTask,
                                TopologyTransition::ResetDevice}) {
            ASSERT_EQ(topo.ChangeState(transition).rc, fair::mq::AsyncOpResultCode::Ok);
        }
    }
}
}   // namespace

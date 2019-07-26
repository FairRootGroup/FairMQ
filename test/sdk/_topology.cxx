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
    dds::topology_api::CTopology nativeTopo(fair::mq::tools::ToString(SDK_TESTSUITE_SOURCE_DIR, "/test_topo.xml"));
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
    topo.ChangeState(TopologyTransition::InitDevice, [&blocker, &topo](Topology::ChangeStateResult result) {
        LOG(info) << result;
        EXPECT_EQ(result.rc, fair::mq::AsyncOpResultCode::Ok);
        EXPECT_NO_THROW(fair::mq::sdk::AggregateState(result.state));
        EXPECT_EQ(fair::mq::sdk::StateEqualsTo(result.state, fair::mq::sdk::DeviceState::Running), true);
        blocker.Signal();
    });
    blocker.Wait();
}

TEST_F(Topology, ChangeStateSync)
{
    using fair::mq::sdk::Topology;
    using fair::mq::sdk::TopologyTransition;

    Topology topo(mDDSTopo, mDDSSession);
    EXPECT_EQ(topo.ChangeState(TopologyTransition::Run).rc, fair::mq::AsyncOpResultCode::Ok);
    auto result(topo.ChangeState(TopologyTransition::Stop));

    EXPECT_EQ(result.rc, fair::mq::AsyncOpResultCode::Ok);
    EXPECT_NO_THROW(fair::mq::sdk::AggregateState(result.state));
    EXPECT_EQ(fair::mq::sdk::StateEqualsTo(result.state, fair::mq::sdk::DeviceState::Ready), true);
}

TEST_F(Topology, ChangeStateConcurrent)
{
    using fair::mq::sdk::Topology;
    using fair::mq::sdk::TopologyTransition;

    Topology topo(mDDSTopo, mDDSSession);
    fair::mq::tools::Semaphore blocker;
    topo.ChangeState(TopologyTransition::Run, [&blocker](Topology::ChangeStateResult result) {
        LOG(info) << "result for valid ChangeState: " << result;
        blocker.Signal();
    });
    topo.ChangeState(TopologyTransition::Stop, [&blocker](Topology::ChangeStateResult result) {
        LOG(info) << "result for invalid ChangeState: " << result;
        EXPECT_EQ(result.rc, fair::mq::AsyncOpResultCode::Error);
    });
    blocker.Wait();
}

TEST_F(Topology, ChangeStateTimeout)
{
    using fair::mq::sdk::Topology;
    using fair::mq::sdk::TopologyTransition;

    Topology topo(mDDSTopo, mDDSSession);
    fair::mq::tools::Semaphore blocker;
    topo.ChangeState(TopologyTransition::End, [&](Topology::ChangeStateResult result) {
        LOG(info) << result;
        EXPECT_EQ(result.rc, fair::mq::AsyncOpResultCode::Timeout);
        blocker.Signal();
    }, std::chrono::milliseconds(1));
    blocker.Wait();
}

}   // namespace

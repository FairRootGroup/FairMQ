/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "TopologyFixture.h"

#include <DDS/Topology.h>
#include <fairmq/sdk/Topology.h>
#include <fairmq/Tools.h>

namespace {

using Topology = fair::mq::test::TopologyFixture;

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
    topo.ChangeState(TopologyTransition::Stop, [&blocker, &topo](Topology::ChangeStateResult result) {
        LOG(info) << result;
        EXPECT_EQ(result.rc, fair::mq::AsyncOpResultCode::Ok);
        EXPECT_NO_THROW(fair::mq::sdk::AggregateState(result.state));
        EXPECT_EQ(fair::mq::sdk::StateEqualsTo(result.state, fair::mq::sdk::DeviceState::Ready), true);
        blocker.Signal();
    });
    blocker.Wait();
}

TEST_F(Topology, ChangeStateSync)
{
    using fair::mq::sdk::Topology;
    using fair::mq::sdk::TopologyTransition;

    Topology topo(mDDSTopo, mDDSSession);
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
    topo.ChangeState(TopologyTransition::Stop, [&blocker](Topology::ChangeStateResult result) {
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

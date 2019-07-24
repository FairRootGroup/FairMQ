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

TEST_F(Topology, ChangeState_async1)
{
    using fair::mq::sdk::Topology;
    using fair::mq::sdk::TopologyTransition;

    Topology topo(mDDSTopo, mDDSSession);
    fair::mq::tools::Semaphore blocker;
    topo.ChangeState(TopologyTransition::Stop, [&blocker](Topology::ChangeStateResult result) {
        LOG(info) << result;
        EXPECT_EQ(result.rc, fair::mq::AsyncOpResultCode::Ok);
        // TODO add the helper to check state consistency
        for (const auto& e : result.state) {
            EXPECT_EQ(e.second.state, fair::mq::sdk::DeviceState::Ready);
        }
        blocker.Signal();
    });
    blocker.Wait();
}

TEST_F(Topology, ChangeState_sync)
{
    using fair::mq::sdk::Topology;
    using fair::mq::sdk::TopologyTransition;

    Topology topo(mDDSTopo, mDDSSession);
    auto result(topo.ChangeState(TopologyTransition::Stop));

    EXPECT_EQ(result.rc, fair::mq::AsyncOpResultCode::Ok);
    // TODO add the helper to check state consistency
    for (const auto& e : result.state) {
        EXPECT_EQ(e.second.state, fair::mq::sdk::DeviceState::Ready);
    }
}
// TEST_F(Topology, Timeout)
// {
//     using fair::mq::sdk::Topology;
//     using fair::mq::sdk::TopologyTransition;

//     Topology topo(mDDSTopo, mDDSSession);
//     Topology::ChangeStateResult r;
//     fair::mq::tools::Semaphore blocker;
//     topo.ChangeState(TopologyTransition::End, [&](Topology::ChangeStateResult result) {
//         LOG(info) << result;
//         blocker.Signal();
//     }, std::chrono::milliseconds(100));
//     blocker.Wait();
//     for (const auto& e : r.rc) {
//         EXPECT_EQ(e.second.state, fair::mq::sdk::DeviceState::Ok);
//     }
// }

}   // namespace

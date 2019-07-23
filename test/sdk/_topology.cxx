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

TEST_F(Topology, ChangeState)
{
    using fair::mq::sdk::Topology;
    using fair::mq::sdk::TopologyTransition;

    Topology topo(mDDSTopo, mDDSSession);
    Topology::ChangeStateResult r;
    fair::mq::tools::Semaphore blocker;
    topo.ChangeState(TopologyTransition::Stop, [&](Topology::ChangeStateResult result) {
        LOG(info) << result;
        r = result;
        blocker.Signal();
    });
    blocker.Wait();
    EXPECT_EQ(r.rc, fair::mq::AsyncOpResult::Ok);
    // TODO add the helper to check state consistency
    for (const auto& e : r.state) {
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

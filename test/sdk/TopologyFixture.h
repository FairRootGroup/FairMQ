/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_TOPOLOGYFIXTURE
#define FAIR_MQ_TEST_TOPOLOGYFIXTURE

#include <DDS/Topology.h>
#include <TestEnvironment.h>
#include <fairlogger/Logger.h>
#include <fairmq/sdk/DDSSession.h>
#include <gtest/gtest.h>

namespace fair {
namespace mq {
namespace test {

struct Topology : ::testing::Test
{
    Topology()
        : mDDSTopologyFile(std::string(SDK_TESTSUITE_SOURCE_DIR) + "/test_topo.xml")
        , mDDSEnv(CMAKE_CURRENT_BINARY_DIR)
        , mDDSSession(mDDSEnv)
    {
        Logger::SetConsoleSeverity("debug");
        Logger::DefineVerbosity("user1",
                                fair::VerbositySpec::Make(VerbositySpec::Info::timestamp_us,
                                                          VerbositySpec::Info::severity));
        Logger::SetVerbosity("user1");
        Logger::SetConsoleColor();
    }
//
    // auto WaitForIdleDDSAgents(int required) -> void {
        // LOG(debug) << "WaitForIdleDDSAgents(" << required << ")";
//
				// DDS Agent Info request
        // dds::tools_api::SAgentInfoRequestData agentInfoInfo;
        // auto agentInfoRequest = dds::tools_api::SAgentInfoRequest::makeRequest(agentInfoInfo);
        // agentInfoRequest->setResponseCallback(
            // [&](const dds::tools_api::SAgentInfoResponseData& _response) {
                // LOG(debug) << "agent: " << _response.m_index << "/" << _response.m_activeAgentsCount;
                // LOG(debug) << "info: " << _response.m_agentInfo;
            // });
        // agentInfoRequest->setMessageCallback(
            // [](const dds::tools_api::SMessageResponseData& _message) { LOG(debug) << _message; });
        // agentInfoRequest->setDoneCallback([&]() {
            // mActiveDDSOps.Signal();
        // });
        // mDDSSession.sendRequest<dds::tools_api::SAgentInfoRequest>(agentInfoRequest);
        // mActiveDDSOps.Wait();
    // }
//
    // auto ActivateDDSTopology(const std::string& topology_file) -> void {
        // LOG(debug) << "ActivateDDSTopology(\"" << topology_file << "\")";
    // }

    auto SetUp() -> void override {
        LOG(info) << mDDSEnv;
        mDDSSession.SubmitAgents(1);
        mDDSSession.SubmitAgents(1);
    }

    auto TearDown() -> void override {}

    std::string mDDSTopologyFile;
    sdk::DDSEnvironment mDDSEnv;
    sdk::DDSSession mDDSSession;
};

} /* namespace test */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TEST_TOPOLOGYFIXTURE */


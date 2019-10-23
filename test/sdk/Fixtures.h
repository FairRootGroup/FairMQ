/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TEST_FIXTURES
#define FAIR_MQ_TEST_FIXTURES

#include "TestEnvironment.h"

#include <asio/io_context.hpp>
#include <chrono>
#include <cstdlib>
#include <fairlogger/Logger.h>
#include <fairmq/SDK.h>
#include <fairmq/tools/Strings.h>
#include <gtest/gtest.h>
#include <thread>

namespace fair {
namespace mq {
namespace test {

struct LoggerConfig
{
    LoggerConfig()
    {
        Logger::SetConsoleSeverity("debug");
        Logger::DefineVerbosity("user1",
                                fair::VerbositySpec::Make(VerbositySpec::Info::timestamp_us,
                                                          VerbositySpec::Info::severity));
        Logger::SetVerbosity("user1");
        Logger::SetConsoleColor();

        std::string path(std::getenv("PATH"));
        path = tools::ToString(FAIRMQ_TEST_ENVIRONMENT, ":", path);
        setenv("PATH", path.c_str(), 1);
    }
};

struct TopologyFixture : ::testing::Test
{
    TopologyFixture()
        : mDDSTopoFile(tools::ToString(SDK_TESTSUITE_SOURCE_DIR, "/test_topo.xml"))
        , mDDSEnv(CMAKE_CURRENT_BINARY_DIR)
        , mDDSSession(mDDSEnv)
        , mDDSTopo(sdk::DDSTopology::Path(mDDSTopoFile), mDDSEnv)
    {
        mDDSSession.StopOnDestruction();
    }

    auto SetUp() -> void override {
        LOG(info) << mDDSEnv;
        LOG(info) << mDDSSession;
        LOG(info) << mDDSTopo;
        auto n(mDDSTopo.GetNumRequiredAgents());
        mDDSSession.SubmitAgents(n);
        mDDSSession.ActivateTopology(mDDSTopo);

        std::vector<sdk::DDSAgent> agents = mDDSSession.RequestAgentInfo();
        LOG(debug) << "##### AgentInfo:";
        LOG(debug) << "size: " << agents.size();
        for (const auto& a : agents) {
            LOG(debug) << a;
        }

        std::vector<sdk::DDSTask> tasks = mDDSSession.RequestTaskInfo();
        LOG(debug) << "##### TaskInfo:";
        LOG(debug) << "size: " << tasks.size();
        for (const auto& t : tasks) {
            LOG(debug) << t;
        }

        std::vector<sdk::DDSCollection> collections = mDDSTopo.GetCollections();
        LOG(debug) << "##### CollectionInfo:";
        LOG(debug) << "size: " << collections.size();
        for (const auto& c : collections) {
            LOG(debug) << c;
        }
    }

    auto TearDown() -> void override {
    }

    LoggerConfig mLoggerConfig;
    std::string mDDSTopoFile;
    sdk::DDSEnvironment mDDSEnv;
    sdk::DDSSession mDDSSession;
    sdk::DDSTopology mDDSTopo;
    asio::io_context mIoContext;
};

struct AsyncOpFixture : ::testing::Test
{
    auto SetUp() -> void override {
    }

    auto TearDown() -> void override {
    }

    LoggerConfig mLoggerConfig;
    asio::io_context mIoContext;
};

} /* namespace test */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_TEST_FIXTURES */


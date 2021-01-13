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

#include <fairmq/SDK.h>
#include <fairmq/tools/Strings.h>

#include <fairlogger/Logger.h>

#include <asio/io_context.hpp>
#include <gtest/gtest.h>

#include <algorithm> // for_each
#include <array>
#include <chrono>
#include <cstdlib>
#include <thread>

namespace fair::mq::test
{

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

    auto TearDown() -> void override {}

    LoggerConfig mLoggerConfig;
    std::string mDDSTopoFile;
    sdk::DDSEnvironment mDDSEnv;
    sdk::DDSSession mDDSSession;
    sdk::DDSTopology mDDSTopo;
    asio::io_context mIoContext;
};

struct MultipleTopologiesFixture : ::testing::Test
{
    MultipleTopologiesFixture()
        : mDDSTopoFile(tools::ToString(SDK_TESTSUITE_SOURCE_DIR, "/test_topo.xml"))
        , mDDSEnv(CMAKE_CURRENT_BINARY_DIR)
        , mDDSSessions{ sdk::DDSSession(mDDSEnv),
                        sdk::DDSSession(mDDSEnv) }
        , mDDSTopologies{ sdk::DDSTopology(sdk::DDSTopology::Path(mDDSTopoFile), mDDSEnv),
                          sdk::DDSTopology(sdk::DDSTopology::Path(mDDSTopoFile), mDDSEnv) }
    {
        std::for_each(mDDSSessions.begin(), mDDSSessions.end(), [](sdk::DDSSession& s) {
            s.StopOnDestruction();
        });
    }

    auto SetUp() -> void override
    {
        LOG(info) << mDDSEnv;
        for (int i = 0; i < mNumSessions; ++i) {
            LOG(info) << "##### SESSION " << i << " #####";
            LOG(info) << mDDSSessions[i];
            LOG(info) << mDDSTopologies[i];
            auto n(mDDSTopologies[i].GetNumRequiredAgents());
            mDDSSessions[i].SubmitAgents(n);
            mDDSSessions[i].ActivateTopology(mDDSTopologies[i]);

            std::vector<sdk::DDSAgent> agents = mDDSSessions[i].RequestAgentInfo();
            LOG(info) << "##### AgentInfo:";
            LOG(info) << "size: " << agents.size();
            for (const auto& a : agents) {
                LOG(info) << a;
            }

            std::vector<sdk::DDSTask> tasks = mDDSSessions[i].RequestTaskInfo();
            LOG(info) << "##### TaskInfo:";
            LOG(info) << "size: " << tasks.size();
            for (const auto& t : tasks) {
                LOG(info) << t;
            }

            std::vector<sdk::DDSCollection> collections = mDDSTopologies[i].GetCollections();
            LOG(info) << "##### CollectionInfo:";
            LOG(info) << "size: " << collections.size();
            for (const auto& c : collections) {
                LOG(info) << c;
            }
        }
    }

    auto TearDown() -> void override {}

    static constexpr int mNumSessions = 2;
    LoggerConfig mLoggerConfig;
    std::string mDDSTopoFile;
    sdk::DDSEnvironment mDDSEnv;
    std::array<sdk::DDSSession, mNumSessions> mDDSSessions;
    std::array<sdk::DDSTopology, mNumSessions> mDDSTopologies;
};

struct AsyncOpFixture : ::testing::Test
{
    auto SetUp() -> void override {}
    auto TearDown() -> void override {}

    LoggerConfig mLoggerConfig;
    asio::io_context mIoContext;
};

} // namespace fair::mq::test

#endif /* FAIR_MQ_TEST_FIXTURES */

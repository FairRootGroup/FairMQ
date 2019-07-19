/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <TestEnvironment.h>
#include <fairlogger/Logger.h>
#include <fairmq/sdk/DDSEnvironment.h>
#include <fairmq/sdk/DDSInfo.h>
#include <fairmq/sdk/DDSSession.h>
#include <gtest/gtest.h>

namespace {

auto setup() -> void
{
    fair::Logger::SetConsoleSeverity("debug");
    fair::Logger::DefineVerbosity("user1",
                                fair::VerbositySpec::Make(fair::VerbositySpec::Info::timestamp_us,
                                                          fair::VerbositySpec::Info::severity));
    fair::Logger::SetVerbosity("user1");
    fair::Logger::SetConsoleColor();
}

TEST(DDS, Environment)
{
    setup();

    fair::mq::sdk::DDSEnvironment env(CMAKE_CURRENT_BINARY_DIR);
    LOG(debug) << env;
}

TEST(DDS, Session)
{
    setup();

    fair::mq::sdk::DDSEnvironment env(CMAKE_CURRENT_BINARY_DIR);
    fair::mq::sdk::DDSSession session(env);
    session.StopOnDestruction();
    LOG(debug) << session;
}

}   // namespace

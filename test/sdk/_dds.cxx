/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Fixtures.h"

#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <dds/dds.h>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS

namespace {

TEST(DDSEnvironment, Construction)
{
    fair::mq::test::LoggerConfig cfg;
    fair::mq::sdk::DDSEnvironment env(CMAKE_CURRENT_BINARY_DIR);

    LOG(debug) << env;
}

TEST(DDSSession, Construction)
{
    fair::mq::test::LoggerConfig cfg;
    fair::mq::sdk::DDSEnvironment env(CMAKE_CURRENT_BINARY_DIR);

    fair::mq::sdk::DDSSession session(env);
    session.StopOnDestruction();
    LOG(debug) << session;
}

TEST(DDSSession, Construction2)
{
    fair::mq::test::LoggerConfig cfg;
    fair::mq::sdk::DDSEnvironment env(CMAKE_CURRENT_BINARY_DIR);

    auto nativeSession(std::make_shared<dds::tools_api::CSession>());
    nativeSession->create();

    fair::mq::sdk::DDSSession session(nativeSession, env);
    session.StopOnDestruction();
    LOG(debug) << session;

    session.RequestCommanderInfo();
}

}   // namespace

/********************************************************************************
 * Copyright (C) 2018-2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tools/Network.h>
#include <gtest/gtest.h>
#include <string>

namespace
{

TEST(Tools, NetworkDefaultIP)
{
    auto const interface = fair::mq::tools::getDefaultRouteNetworkInterface();
    EXPECT_NE(interface, "");
    auto const interfaceIP = fair::mq::tools::getInterfaceIP(interface);
    EXPECT_NE(interfaceIP, "");
}

TEST(Tools, NetworkIPv4Localhost)
{
    auto const ip = fair::mq::tools::getIpFromHostname("localhost");
    EXPECT_FALSE(ip.empty());
    EXPECT_EQ(ip, "127.0.0.1");
}

TEST(Tools, NetworkInvalidHostname)
{
    auto const ip = fair::mq::tools::getIpFromHostname("non.existent.domain.invalid");
    EXPECT_TRUE(ip.empty());
}

} /* namespace */

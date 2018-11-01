/********************************************************************************
 *    Copyright (C) 2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <gtest/gtest.h>
#include <fairmq/Tools.h>

#include <string>

namespace
{

using namespace std;
using namespace fair::mq;

TEST(Tools, Network)
{
    string interface = fair::mq::tools::getDefaultRouteNetworkInterface();
    EXPECT_NE(interface, "");
    string interfaceIP = fair::mq::tools::getInterfaceIP(interface);
    EXPECT_NE(interfaceIP, "");
}

} /* namespace */

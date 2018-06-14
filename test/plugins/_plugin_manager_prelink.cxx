/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <gtest/gtest.h>
#include <fairmq/PluginManager.h>

namespace
{

using namespace fair::mq;
using namespace std;

TEST(PluginManager, LoadPluginPrelinkedDynamic)
{
    PluginManager mgr;

    ASSERT_NO_THROW(mgr.LoadPlugin("p:test_dummy"));
    ASSERT_NO_THROW(mgr.LoadPlugin("p:test_dummy2"));
}

} /* namespace */

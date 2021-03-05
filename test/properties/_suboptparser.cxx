/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Properties.h>
#include <fairmq/SuboptParser.h>
#include <gtest/gtest.h>
#include <string>
#include <string_view>

namespace {

using namespace std;
using namespace fair::mq;

auto contains(Properties const& parsed, string_view key, string_view value) -> bool
{
    return PropertyHelper::ConvertPropertyToString(parsed.at(string(key))) == value;
}

TEST(SuboptParser, ChannelNameSelector)
{
    Properties parsed(SuboptParser({"name=foo-data,address=tcp://0.0.0.0:6000,type=push",
                                    "bar-data:address=tcp://0.0.0.0:7000,type=pull"},
                                   "foo"));
    ASSERT_TRUE(contains(parsed, "chans.foo-data.0.address", "tcp://0.0.0.0:6000"));
    ASSERT_TRUE(contains(parsed, "chans.foo-data.0.type", "push"));
    ASSERT_TRUE(contains(parsed, "chans.bar-data.0.address", "tcp://0.0.0.0:7000"));
    ASSERT_TRUE(contains(parsed, "chans.bar-data.0.type", "pull"));
}

}   // namespace

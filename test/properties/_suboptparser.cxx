/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Properties.h>
#include <fairmq/JSONParser.h>
#include <fairmq/SuboptParser.h>
#include <boost/property_tree/exceptions.hpp>
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

TEST(SuboptParser, AllKeys)
{
    Properties const parsed(SuboptParser({"name=data,type=pub,method=bind,address=ipc://a,transport=shmem,"
                                          "sndBufSize=1,rcvBufSize=2,sndKernelSize=3,rcvKernelSize=4,"
                                          "sndTimeoutMs=5,rcvTimeoutMs=6,linger=7,rateLogging=8,"
                                          "portRangeMin=9,portRangeMax=10,autoBind=0"},
                                         "dev"));
    auto get = [&](string const& key) { return parsed.at("chans.data.0." + key); };
    EXPECT_EQ(boost::any_cast<string>(get("type")), "pub");
    EXPECT_EQ(boost::any_cast<string>(get("method")), "bind");
    EXPECT_EQ(boost::any_cast<string>(get("address")), "ipc://a");
    EXPECT_EQ(boost::any_cast<string>(get("transport")), "shmem");
    EXPECT_EQ(boost::any_cast<int>(get("sndBufSize")), 1);
    EXPECT_EQ(boost::any_cast<int>(get("rcvBufSize")), 2);
    EXPECT_EQ(boost::any_cast<int>(get("sndKernelSize")), 3);
    EXPECT_EQ(boost::any_cast<int>(get("rcvKernelSize")), 4);
    EXPECT_EQ(boost::any_cast<int>(get("sndTimeoutMs")), 5);
    EXPECT_EQ(boost::any_cast<int>(get("rcvTimeoutMs")), 6);
    EXPECT_EQ(boost::any_cast<int>(get("linger")), 7);
    EXPECT_EQ(boost::any_cast<int>(get("rateLogging")), 8);
    EXPECT_EQ(boost::any_cast<int>(get("portRangeMin")), 9);
    EXPECT_EQ(boost::any_cast<int>(get("portRangeMax")), 10);
    EXPECT_FALSE(boost::any_cast<bool>(get("autoBind")));
}

TEST(SuboptParser, OneSubChannelPerAddress)
{
    Properties const parsed(SuboptParser({"name=data,type=push,address=ipc://a,address=ipc://b"}, "dev"));
    EXPECT_TRUE(contains(parsed, "chans.data.0.address", "ipc://a"));
    EXPECT_TRUE(contains(parsed, "chans.data.1.address", "ipc://b"));
    EXPECT_TRUE(contains(parsed, "chans.data.1.type", "push"));
    EXPECT_EQ(parsed.count("chans.data.2.address"), 0);
}

TEST(SuboptParser, NumSockets)
{
    Properties const parsed(SuboptParser({"name=data,type=push,numSockets=2"}, "dev"));
    EXPECT_TRUE(contains(parsed, "chans.data.0.type", "push"));
    EXPECT_TRUE(contains(parsed, "chans.data.1.type", "push"));
    EXPECT_EQ(parsed.count("chans.data.2.type"), 0);
}

TEST(SuboptParser, IgnoresUnknownKeys)
{
    Properties const parsed(SuboptParser({"name=data,colour=blue,type=pull"}, "dev"));
    EXPECT_TRUE(contains(parsed, "chans.data.0.type", "pull"));
}

TEST(SuboptParser, Errors)
{
    // No channel name.
    EXPECT_THROW(SuboptParser({"type=push,address=ipc://a"}, "dev"), boost::property_tree::ptree_bad_path);
    // No device id.
    EXPECT_THROW(SuboptParser({"name=data"}, ""), ParserError);
}

// Known defect: see JSONParser.DISABLED_RejectsValuesOfTheWrongType.
TEST(SuboptParser, DISABLED_RejectsValuesOfTheWrongType)
{
    EXPECT_THROW(SuboptParser({"name=data,sndBufSize=big"}, "dev"), boost::property_tree::ptree_bad_data);
}

}   // namespace

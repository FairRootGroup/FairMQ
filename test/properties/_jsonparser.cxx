/********************************************************************************
 *    Copyright (C) 2026 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Channel.h>
#include <fairmq/JSONParser.h>
#include <fairmq/Properties.h>
#include <fairmq/tools/Unique.h>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace
{

using namespace std;
using namespace fair::mq;
using boost::property_tree::ptree;

auto Parse(string const& json, string const& id) -> Properties
{
    ptree pt;
    stringstream ss(json);
    boost::property_tree::read_json(ss, pt);
    return PtreeParser(pt, id);
}

template<typename T>
auto Get(Properties const& props, string const& key) -> T
{
    return boost::any_cast<T>(props.at(key));
}

auto const kConfig = R"({
    "fairMQOptions": {
        "devices": [
            {
                "id": "sampler",
                "channels": [
                    {
                        "name": "data",
                        "type": "push",
                        "method": "bind",
                        "transport": "zeromq",
                        "sndBufSize": 500,
                        "sockets": [
                            { "address": "tcp://127.0.0.1:5555" },
                            { "address": "ipc://data", "transport": "shmem", "method": "connect", "rateLogging": 0 }
                        ]
                    },
                    {
                        "name": "fanout",
                        "type": "push",
                        "address": "tcp://127.0.0.1:5556",
                        "numSockets": 3
                    },
                    {
                        "name": "plain",
                        "type": "pair",
                        "autoBind": false
                    }
                ]
            },
            {
                "id": "other",
                "key": "sink",
                "channels": [ { "name": "data", "type": "pull" } ]
            }
        ]
    }
})";

TEST(JSONParser, SubChannelsInheritAndOverride)
{
    auto const props = Parse(kConfig, "sampler");

    EXPECT_EQ(Get<string>(props, "chans.data.0.type"), "push");
    EXPECT_EQ(Get<string>(props, "chans.data.0.method"), "bind");
    EXPECT_EQ(Get<string>(props, "chans.data.0.address"), "tcp://127.0.0.1:5555");
    EXPECT_EQ(Get<string>(props, "chans.data.0.transport"), "zeromq");
    EXPECT_EQ(Get<int>(props, "chans.data.0.sndBufSize"), 500);
    EXPECT_EQ(Get<int>(props, "chans.data.0.rateLogging"), Channel::DefaultRateLogging);

    EXPECT_EQ(Get<string>(props, "chans.data.1.type"), "push");
    EXPECT_EQ(Get<string>(props, "chans.data.1.method"), "connect");
    EXPECT_EQ(Get<string>(props, "chans.data.1.address"), "ipc://data");
    EXPECT_EQ(Get<string>(props, "chans.data.1.transport"), "shmem");
    EXPECT_EQ(Get<int>(props, "chans.data.1.sndBufSize"), 500);
    EXPECT_EQ(Get<int>(props, "chans.data.1.rateLogging"), 0);

    EXPECT_EQ(props.count("chans.data.2.address"), 0);
}

TEST(JSONParser, NumSocketsExpandsTheChannel)
{
    auto const props = Parse(kConfig, "sampler");

    for (int i = 0; i < 3; ++i) {
        auto const prefix = "chans.fanout." + to_string(i) + ".";
        EXPECT_EQ(Get<string>(props, prefix + "type"), "push");
        EXPECT_EQ(Get<string>(props, prefix + "address"), "tcp://127.0.0.1:5556");
    }
    EXPECT_EQ(props.count("chans.fanout.3.address"), 0);
}

TEST(JSONParser, ChannelWithoutSocketsGetsTheDefaults)
{
    auto const props = Parse(kConfig, "sampler");

    EXPECT_EQ(Get<string>(props, "chans.plain.0.type"), "pair");
    EXPECT_EQ(Get<string>(props, "chans.plain.0.method"), Channel::DefaultMethod);
    EXPECT_EQ(Get<string>(props, "chans.plain.0.address"), Channel::DefaultAddress);
    EXPECT_EQ(Get<string>(props, "chans.plain.0.transport"), Channel::DefaultTransportName);
    EXPECT_EQ(Get<int>(props, "chans.plain.0.sndBufSize"), Channel::DefaultSndBufSize);
    EXPECT_EQ(Get<int>(props, "chans.plain.0.rcvBufSize"), Channel::DefaultRcvBufSize);
    EXPECT_EQ(Get<int>(props, "chans.plain.0.sndKernelSize"), Channel::DefaultSndKernelSize);
    EXPECT_EQ(Get<int>(props, "chans.plain.0.rcvKernelSize"), Channel::DefaultRcvKernelSize);
    EXPECT_EQ(Get<int>(props, "chans.plain.0.sndTimeoutMs"), Channel::DefaultSndTimeoutMs);
    EXPECT_EQ(Get<int>(props, "chans.plain.0.rcvTimeoutMs"), Channel::DefaultRcvTimeoutMs);
    EXPECT_EQ(Get<int>(props, "chans.plain.0.linger"), Channel::DefaultLinger);
    EXPECT_EQ(Get<int>(props, "chans.plain.0.portRangeMin"), Channel::DefaultPortRangeMin);
    EXPECT_EQ(Get<int>(props, "chans.plain.0.portRangeMax"), Channel::DefaultPortRangeMax);
    EXPECT_FALSE(Get<bool>(props, "chans.plain.0.autoBind"));
}

TEST(JSONParser, SelectsTheDeviceByKeyOrId)
{
    auto const sink = Parse(kConfig, "sink");
    EXPECT_EQ(Get<string>(sink, "chans.data.0.type"), "pull");
    EXPECT_EQ(sink.count("chans.fanout.0.type"), 0);

    // With a key, the id does not select the device.
    EXPECT_TRUE(Parse(kConfig, "other").empty());
    EXPECT_TRUE(Parse(kConfig, "unknown").empty());
}

TEST(JSONParser, Errors)
{
    EXPECT_THROW(Parse(kConfig, ""), ParserError);
    EXPECT_THROW(Parse(R"({"options": {}})", "sampler"), boost::property_tree::ptree_bad_path);
    // Channel without a name.
    EXPECT_THROW(Parse(R"({"fairMQOptions": {"devices": [{"id": "a", "channels": [{"type": "push"}]}]}})", "a"),
                 boost::property_tree::ptree_bad_path);
}

// A typo (e.g. "sndBufSize": "big") must not silently give the default.
TEST(JSONParser, RejectsValuesOfTheWrongType)
{
    EXPECT_THROW(Parse(R"({"fairMQOptions": {"devices": [{"id": "a", "channels": [{"name": "c", "sndBufSize": "big"}]}]}})", "a"),
                 ParserError);
    EXPECT_THROW(Parse(R"({"fairMQOptions": {"devices": [{"id": "a", "channels": [{"name": "c", "autoBind": "maybe"}]}]}})", "a"),
                 ParserError);
    EXPECT_THROW(Parse(R"({"fairMQOptions": {"devices": [{"id": "a", "channels": [{"name": "c", "sockets": [{"linger": "1.5"}]}]}]}})", "a"),
                 ParserError);
}

TEST(JSONParser, File)
{
    auto const path = "/tmp/fmq_test_jsonparser_" + to_string(tools::UuidHash()) + ".json";
    {
        ofstream file(path);
        file << kConfig;
    }
    auto const props = JSONParser(path, "sampler");
    remove(path.c_str());

    EXPECT_EQ(Get<string>(props, "chans.data.1.address"), "ipc://data");
    EXPECT_THROW(JSONParser(path, "sampler"), boost::property_tree::json_parser_error);
}

} // namespace

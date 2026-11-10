/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/*
 * File:   JSONParser.cxx
 * Author: winckler
 *
 * Created on May 14, 2015, 5:01 PM
 */

#include "JSONParser.h"

#include <fairlogger/Logger.h>

#include <boost/any.hpp>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/property_tree/json_parser.hpp>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/property_tree/ptree.hpp>
#include <fairlogger/Logger.h>
#include <fairmq/Channel.h>
#include <fairmq/JSONParser.h>
#include <fairmq/PropertyOutput.h>
#include <fairmq/tools/Strings.h>
#include <iomanip>

using namespace std;
using namespace fair::mq;
using namespace tools;
using namespace boost::property_tree;

namespace fair::mq
{

Properties PtreeParser(const ptree& pt, const string& id)
{
    if (id.empty()) {
        throw ParserError("no device ID provided. Provide with `--id` cmd option");
    }

    // json_parser::write_json(cout, pt);

    return helper::DeviceParser(pt.get_child("fairMQOptions"), id);
}

Properties JSONParser(const string& filename, const string& deviceId)
{
    ptree pt;
    LOG(debug) << "Parsing JSON from " << filename << " ...";
    read_json(filename, pt);
    return PtreeParser(pt, deviceId);
}

namespace helper
{

namespace
{

// ptree::get(key, default) also returns the default for a value of the wrong type.
// Use the default only for a missing key.
template<typename T>
T Get(const ptree& tree, const string& key, const T& defaultValue)
{
    auto const child = tree.get_child_optional(key);
    if (!child) {
        return defaultValue;
    }
    try {
        return child->get_value<T>();
    } catch (ptree_bad_data const&) {
        throw ParserError(ToString("Invalid value '", child->data(), "' for channel property '", key, "'"));
    }
}

} // namespace

Properties DeviceParser(const ptree& fairMQOptions, const string& deviceId)
{
    Properties properties;

    for (const auto& node : fairMQOptions) {
        if (node.first == "devices") {
            for (const auto& device : node.second) {
                // check if key is provided, otherwise use id
                string deviceIdKey = device.second.get<string>("key", device.second.get<string>("id", ""));

                // if not correct device id, do not fill MQMap
                if (deviceId != deviceIdKey) {
                    continue;
                }

                LOG(trace) << "Found following channels for device ID '" << deviceId << "' :";
                ChannelParser(device.second, properties);
            }
        }
    }

    return properties;
}

void ChannelParser(const ptree& tree, Properties& properties)
{
    for (const auto& node : tree) {
        if (node.first == "channels") {
            for (const auto& cn : node.second) {
                Properties commonProperties;
                commonProperties.emplace("type", cn.second.get<string>("type", Channel::DefaultType));
                commonProperties.emplace("method", cn.second.get<string>("method", Channel::DefaultMethod));
                commonProperties.emplace("address", cn.second.get<string>("address", Channel::DefaultAddress));
                commonProperties.emplace("transport", cn.second.get<string>("transport", Channel::DefaultTransportName));
                commonProperties.emplace("sndBufSize", Get<int>(cn.second, "sndBufSize", Channel::DefaultSndBufSize));
                commonProperties.emplace("rcvBufSize", Get<int>(cn.second, "rcvBufSize", Channel::DefaultRcvBufSize));
                commonProperties.emplace("sndKernelSize", Get<int>(cn.second, "sndKernelSize", Channel::DefaultSndKernelSize));
                commonProperties.emplace("rcvKernelSize", Get<int>(cn.second, "rcvKernelSize", Channel::DefaultRcvKernelSize));
                commonProperties.emplace("sndTimeoutMs", Get<int>(cn.second, "sndTimeoutMs", Channel::DefaultSndTimeoutMs));
                commonProperties.emplace("rcvTimeoutMs", Get<int>(cn.second, "rcvTimeoutMs", Channel::DefaultRcvTimeoutMs));
                commonProperties.emplace("linger", Get<int>(cn.second, "linger", Channel::DefaultLinger));
                commonProperties.emplace("rateLogging", Get<int>(cn.second, "rateLogging", Channel::DefaultRateLogging));
                commonProperties.emplace("portRangeMin", Get<int>(cn.second, "portRangeMin", Channel::DefaultPortRangeMin));
                commonProperties.emplace("portRangeMax", Get<int>(cn.second, "portRangeMax", Channel::DefaultPortRangeMax));
                commonProperties.emplace("autoBind", Get<bool>(cn.second, "autoBind", Channel::DefaultAutoBind));

                string name = cn.second.get<string>("name");
                int numSockets = Get<int>(cn.second, "numSockets", 0);

                if (numSockets > 0) {
                    LOG(trace) << name << ":";
                    LOG(trace) << "\tnumSockets of " << numSockets << " specified, applying common settings to each:";

                    for (auto& p : commonProperties) {
                        LOG(trace) << "\t" << setw(13) << left << p.first << " : " << p.second;
                    }

                    for (int i = 0; i < numSockets; ++i) {
                        for (const auto& p : commonProperties) {
                            properties.emplace(ToString("chans.", name, ".", i, ".", p.first), p.second);
                        }
                    }
                } else {
                    SubChannelParser(cn.second.get_child(""), properties, name, commonProperties);
                }
            }
        }
    }
}

void SubChannelParser(const ptree& channelTree, Properties& properties, const string& channelName, const Properties& commonProperties)
{
    // for each socket in channel
    int i = 0;

    for (const auto& node : channelTree) {
        if (node.first == "sockets") {
            for (const auto& sn : node.second) {
                // a sub-channel inherits relevant properties from the common channel ...
                Properties newProperties(commonProperties);

                // ... and adds/overwrites its own properties
                newProperties["type"] = sn.second.get<string>("type", boost::any_cast<string>(commonProperties.at("type")));
                newProperties["method"] = sn.second.get<string>("method", boost::any_cast<string>(commonProperties.at("method")));
                newProperties["address"] = sn.second.get<string>("address", boost::any_cast<string>(commonProperties.at("address")));
                newProperties["transport"] = sn.second.get<string>("transport", boost::any_cast<string>(commonProperties.at("transport")));
                newProperties["sndBufSize"] = Get<int>(sn.second, "sndBufSize", boost::any_cast<int>(commonProperties.at("sndBufSize")));
                newProperties["rcvBufSize"] = Get<int>(sn.second, "rcvBufSize", boost::any_cast<int>(commonProperties.at("rcvBufSize")));
                newProperties["sndKernelSize"] = Get<int>(sn.second, "sndKernelSize", boost::any_cast<int>(commonProperties.at("sndKernelSize")));
                newProperties["rcvKernelSize"] = Get<int>(sn.second, "rcvKernelSize", boost::any_cast<int>(commonProperties.at("rcvKernelSize")));
                newProperties["sndTimeoutMs"] = Get<int>(sn.second, "sndTimeoutMs", boost::any_cast<int>(commonProperties.at("sndTimeoutMs")));
                newProperties["rcvTimeoutMs"] = Get<int>(sn.second, "rcvTimeoutMs", boost::any_cast<int>(commonProperties.at("rcvTimeoutMs")));
                newProperties["linger"] = Get<int>(sn.second, "linger", boost::any_cast<int>(commonProperties.at("linger")));
                newProperties["rateLogging"] = Get<int>(sn.second, "rateLogging", boost::any_cast<int>(commonProperties.at("rateLogging")));
                newProperties["portRangeMin"] = Get<int>(sn.second, "portRangeMin", boost::any_cast<int>(commonProperties.at("portRangeMin")));
                newProperties["portRangeMax"] = Get<int>(sn.second, "portRangeMax", boost::any_cast<int>(commonProperties.at("portRangeMax")));
                newProperties["autoBind"] = Get<bool>(sn.second, "autoBind", boost::any_cast<bool>(commonProperties.at("autoBind")));

                LOG(trace) << "" << channelName << "[" << i << "]:";
                for (auto& p : newProperties) {
                    LOG(trace) << "\t" << setw(13) << left << p.first << " : " << p.second;
                }

                for (const auto& p : newProperties) {
                    properties.emplace(ToString("chans.", channelName, ".", i, ".", p.first), p.second);
                }
                ++i;
            }
        }
    }

    if (i > 0) {
        LOG(trace) << "Found " << i << " socket(s) in channel.";
    } else {
        // if no sockets are specified, apply common channel properties
        LOG(trace) << "" << channelName << ":";
        LOG(trace) << "\tNo sockets specified,";
        LOG(trace) << "\tapplying common settings to the channel:";

        Properties newProperties(commonProperties);

        for (auto& p : newProperties) {
            LOG(trace) << "\t" << setw(13) << left << p.first << " : " << p.second;
        }

        for (const auto& p : newProperties) {
            properties.emplace(ToString("chans.", channelName, ".0.", p.first), p.second);
        }
    }
}

} // helper namespace
} // namespace fair::mq

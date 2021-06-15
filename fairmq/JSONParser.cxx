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
#include "FairMQChannel.h"
#include <fairmq/PropertyOutput.h>
#include <fairmq/tools/Strings.h>

#include <fairlogger/Logger.h>

#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/property_tree/json_parser.hpp>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/property_tree/ptree.hpp>
#include <boost/any.hpp>

#include <iomanip>

using namespace std;
using namespace fair::mq;
using namespace fair::mq::tools;
using namespace boost::property_tree;

namespace fair::mq
{

fair::mq::Properties PtreeParser(const ptree& pt, const string& id)
{
    if (id.empty()) {
        throw ParserError("no device ID provided. Provide with `--id` cmd option");
    }

    // json_parser::write_json(cout, pt);

    return helper::DeviceParser(pt.get_child("fairMQOptions"), id);
}

fair::mq::Properties JSONParser(const string& filename, const string& deviceId)
{
    ptree pt;
    LOG(debug) << "Parsing JSON from " << filename << " ...";
    read_json(filename, pt);
    return PtreeParser(pt, deviceId);
}

namespace helper
{

fair::mq::Properties DeviceParser(const ptree& fairMQOptions, const string& deviceId)
{
    fair::mq::Properties properties;

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

void ChannelParser(const ptree& tree, fair::mq::Properties& properties)
{
    for (const auto& node : tree) {
        if (node.first == "channels") {
            for (const auto& cn : node.second) {
                fair::mq::Properties commonProperties;
                commonProperties.emplace("type", cn.second.get<string>("type", FairMQChannel::DefaultType));
                commonProperties.emplace("method", cn.second.get<string>("method", FairMQChannel::DefaultMethod));
                commonProperties.emplace("address", cn.second.get<string>("address", FairMQChannel::DefaultAddress));
                commonProperties.emplace("transport", cn.second.get<string>("transport", FairMQChannel::DefaultTransportName));
                commonProperties.emplace("sndBufSize", cn.second.get<int>("sndBufSize", FairMQChannel::DefaultSndBufSize));
                commonProperties.emplace("rcvBufSize", cn.second.get<int>("rcvBufSize", FairMQChannel::DefaultRcvBufSize));
                commonProperties.emplace("sndKernelSize", cn.second.get<int>("sndKernelSize", FairMQChannel::DefaultSndKernelSize));
                commonProperties.emplace("rcvKernelSize", cn.second.get<int>("rcvKernelSize", FairMQChannel::DefaultRcvKernelSize));
                commonProperties.emplace("linger", cn.second.get<int>("linger", FairMQChannel::DefaultLinger));
                commonProperties.emplace("rateLogging", cn.second.get<int>("rateLogging", FairMQChannel::DefaultRateLogging));
                commonProperties.emplace("portRangeMin", cn.second.get<int>("portRangeMin", FairMQChannel::DefaultPortRangeMin));
                commonProperties.emplace("portRangeMax", cn.second.get<int>("portRangeMax", FairMQChannel::DefaultPortRangeMax));
                commonProperties.emplace("autoBind", cn.second.get<bool>("autoBind", FairMQChannel::DefaultAutoBind));

                string name = cn.second.get<string>("name");
                int numSockets = cn.second.get<int>("numSockets", 0);

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

void SubChannelParser(const ptree& channelTree, fair::mq::Properties& properties, const string& channelName, const fair::mq::Properties& commonProperties)
{
    // for each socket in channel
    int i = 0;

    for (const auto& node : channelTree) {
        if (node.first == "sockets") {
            for (const auto& sn : node.second) {
                // a sub-channel inherits relevant properties from the common channel ...
                fair::mq::Properties newProperties(commonProperties);

                // ... and adds/overwrites its own properties
                newProperties["type"] = sn.second.get<string>("type", boost::any_cast<string>(commonProperties.at("type")));
                newProperties["method"] = sn.second.get<string>("method", boost::any_cast<string>(commonProperties.at("method")));
                newProperties["address"] = sn.second.get<string>("address", boost::any_cast<string>(commonProperties.at("address")));
                newProperties["transport"] = sn.second.get<string>("transport", boost::any_cast<string>(commonProperties.at("transport")));
                newProperties["sndBufSize"] = sn.second.get<int>("sndBufSize", boost::any_cast<int>(commonProperties.at("sndBufSize")));
                newProperties["rcvBufSize"] = sn.second.get<int>("rcvBufSize", boost::any_cast<int>(commonProperties.at("rcvBufSize")));
                newProperties["sndKernelSize"] = sn.second.get<int>("sndKernelSize", boost::any_cast<int>(commonProperties.at("sndKernelSize")));
                newProperties["rcvKernelSize"] = sn.second.get<int>("rcvKernelSize", boost::any_cast<int>(commonProperties.at("rcvKernelSize")));
                newProperties["linger"] = sn.second.get<int>("linger", boost::any_cast<int>(commonProperties.at("linger")));
                newProperties["rateLogging"] = sn.second.get<int>("rateLogging", boost::any_cast<int>(commonProperties.at("rateLogging")));
                newProperties["portRangeMin"] = sn.second.get<int>("portRangeMin", boost::any_cast<int>(commonProperties.at("portRangeMin")));
                newProperties["portRangeMax"] = sn.second.get<int>("portRangeMax", boost::any_cast<int>(commonProperties.at("portRangeMax")));
                newProperties["autoBind"] = sn.second.get<bool>("autoBind", boost::any_cast<bool>(commonProperties.at("autoBind")));

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

        fair::mq::Properties newProperties(commonProperties);

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

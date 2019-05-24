/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/*
 * File:   FairMQParser.cxx
 * Author: winckler
 *
 * Created on May 14, 2015, 5:01 PM
 */

#include "FairMQParser.h"
#include "FairMQLogger.h"
#include <fairmq/Tools.h>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/any.hpp>

using namespace std;
using namespace fair::mq::tools;
using namespace boost::property_tree;

namespace fair
{
namespace mq
{
namespace parser
{

fair::mq::Properties ptreeToProperties(const ptree& pt, const string& id)
{
    if (id == "") {
        throw ParserError("no device ID provided. Provide with `--id` cmd option");
    }

    return Helper::DeviceParser(pt.get_child("fairMQOptions"), id);
}

fair::mq::Properties JSON::UserParser(const string& filename, const string& deviceId)
{
    ptree input;
    LOG(debug) << "Parsing JSON from " << filename << " ...";
    read_json(filename, input);
    return ptreeToProperties(input, deviceId);
}

namespace Helper
{

fair::mq::Properties DeviceParser(const ptree& fairMQOptions, const string& deviceId)
{
    fair::mq::Properties properties;

    for (const auto& node : fairMQOptions) {
        if (node.first == "devices") {
            for (const auto& device : node.second) {
                string deviceIdKey;

                // check if key is provided, otherwise use id
                string key = device.second.get<string>("key", "");

                if (key != "") {
                    deviceIdKey = key;
                } else {
                    deviceIdKey = device.second.get<string>("id");
                }

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

                    // TODO: make a loop out of this
                    LOG(trace) << "\ttype          = " << boost::any_cast<string>(commonProperties.at("type"));
                    LOG(trace) << "\tmethod        = " << boost::any_cast<string>(commonProperties.at("method"));
                    LOG(trace) << "\taddress       = " << boost::any_cast<string>(commonProperties.at("address"));
                    LOG(trace) << "\ttransport     = " << boost::any_cast<string>(commonProperties.at("transport"));
                    LOG(trace) << "\tsndBufSize    = " << boost::any_cast<int>(commonProperties.at("sndBufSize"));
                    LOG(trace) << "\trcvBufSize    = " << boost::any_cast<int>(commonProperties.at("rcvBufSize"));
                    LOG(trace) << "\tsndKernelSize = " << boost::any_cast<int>(commonProperties.at("sndKernelSize"));
                    LOG(trace) << "\trcvKernelSize = " << boost::any_cast<int>(commonProperties.at("rcvKernelSize"));
                    LOG(trace) << "\tlinger        = " << boost::any_cast<int>(commonProperties.at("linger"));
                    LOG(trace) << "\trateLogging   = " << boost::any_cast<int>(commonProperties.at("rateLogging"));
                    LOG(trace) << "\tportRangeMin  = " << boost::any_cast<int>(commonProperties.at("portRangeMin"));
                    LOG(trace) << "\tportRangeMax  = " << boost::any_cast<int>(commonProperties.at("portRangeMax"));
                    LOG(trace) << "\tautoBind      = " << boost::any_cast<bool>(commonProperties.at("autoBind"));

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
                // TODO: make a loop out of this
                LOG(trace) << "\ttype          = " << boost::any_cast<string>(newProperties.at("type"));
                LOG(trace) << "\tmethod        = " << boost::any_cast<string>(newProperties.at("method"));
                LOG(trace) << "\taddress       = " << boost::any_cast<string>(newProperties.at("address"));
                LOG(trace) << "\ttransport     = " << boost::any_cast<string>(newProperties.at("transport"));
                LOG(trace) << "\tsndBufSize    = " << boost::any_cast<int>(newProperties.at("sndBufSize"));
                LOG(trace) << "\trcvBufSize    = " << boost::any_cast<int>(newProperties.at("rcvBufSize"));
                LOG(trace) << "\tsndKernelSize = " << boost::any_cast<int>(newProperties.at("sndKernelSize"));
                LOG(trace) << "\trcvKernelSize = " << boost::any_cast<int>(newProperties.at("rcvKernelSize"));
                LOG(trace) << "\tlinger        = " << boost::any_cast<int>(newProperties.at("linger"));
                LOG(trace) << "\trateLogging   = " << boost::any_cast<int>(newProperties.at("rateLogging"));
                LOG(trace) << "\tportRangeMin  = " << boost::any_cast<int>(newProperties.at("portRangeMin"));
                LOG(trace) << "\tportRangeMax  = " << boost::any_cast<int>(newProperties.at("portRangeMax"));
                LOG(trace) << "\tautoBind      = " << boost::any_cast<bool>(newProperties.at("autoBind"));

                for (const auto& p : newProperties) {
                    properties.emplace(ToString("chans.", channelName, ".", i, ".", p.first), p.second);
                }
                ++i;
            }
        }
    } // end socket loop

    if (i > 0) {
        LOG(trace) << "Found " << i << " socket(s) in channel.";
    } else {
        LOG(trace) << "" << channelName << ":";
        LOG(trace) << "\tNo sockets specified,";
        LOG(trace) << "\tapplying common settings to the channel:";

        fair::mq::Properties newProperties(commonProperties);

        // TODO: make a loop out of this
        LOG(trace) << "\ttype          = " << boost::any_cast<string>(newProperties.at("type"));
        LOG(trace) << "\tmethod        = " << boost::any_cast<string>(newProperties.at("method"));
        LOG(trace) << "\taddress       = " << boost::any_cast<string>(newProperties.at("address"));
        LOG(trace) << "\ttransport     = " << boost::any_cast<string>(newProperties.at("transport"));
        LOG(trace) << "\tsndBufSize    = " << boost::any_cast<int>(newProperties.at("sndBufSize"));
        LOG(trace) << "\trcvBufSize    = " << boost::any_cast<int>(newProperties.at("rcvBufSize"));
        LOG(trace) << "\tsndKernelSize = " << boost::any_cast<int>(newProperties.at("sndKernelSize"));
        LOG(trace) << "\trcvKernelSize = " << boost::any_cast<int>(newProperties.at("rcvKernelSize"));
        LOG(trace) << "\tlinger        = " << boost::any_cast<int>(newProperties.at("linger"));
        LOG(trace) << "\trateLogging   = " << boost::any_cast<int>(newProperties.at("rateLogging"));
        LOG(trace) << "\tportRangeMin  = " << boost::any_cast<int>(newProperties.at("portRangeMin"));
        LOG(trace) << "\tportRangeMax  = " << boost::any_cast<int>(newProperties.at("portRangeMax"));
        LOG(trace) << "\tautoBind      = " << boost::any_cast<bool>(newProperties.at("autoBind"));

        for (const auto& p : newProperties) {
            properties.emplace(ToString("chans.", channelName, ".0.", p.first), p.second);
        }
    }
}

} // Helper namespace

} // namespace parser
} // namespace mq
} // namespace fair

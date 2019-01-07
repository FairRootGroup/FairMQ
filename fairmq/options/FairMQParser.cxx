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

using namespace std;

namespace fair
{
namespace mq
{
namespace parser
{

// TODO : add key-value map<string,string> parameter  for replacing/updating values from keys
// function that convert property tree (given the json structure) to FairMQChannelMap
FairMQChannelMap ptreeToMQMap(const boost::property_tree::ptree& pt, const string& id, const string& rootNode)
{
    if (id == "") {
        throw ParserError("no device ID provided. Provide with `--id` cmd option");
    }

    // Create fair mq map
    FairMQChannelMap channelMap;
    // boost::property_tree::json_parser::write_json(std::cout, pt);
    // Helper::PrintDeviceList(pt.get_child(rootNode));
    // Extract value from boost::property_tree
    Helper::DeviceParser(pt.get_child(rootNode), channelMap, id);

    if (channelMap.empty()) {
        LOG(warn) << "---- No channel keys found for " << id;
        LOG(warn) << "---- Check the JSON inputs and/or command line inputs";
    }

    return channelMap;
}

FairMQChannelMap JSON::UserParser(const string& filename, const string& deviceId, const string& rootNode)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(filename, pt);
    return ptreeToMQMap(pt, deviceId, rootNode);
}

namespace Helper
{

void PrintDeviceList(const boost::property_tree::ptree& tree)
{
    string deviceIdKey;

    // do a first loop just to print the device-id in json input
    for (const auto& p : tree) {
        if (p.first == "devices") {
            for (const auto& q : p.second.get_child("")) {
                string key = q.second.get<string>("key", "");
                if (key != "") {
                    deviceIdKey = key;
                    LOG(debug) << "Found config for device key '" << deviceIdKey << "' in JSON input";
                } else {
                    deviceIdKey = q.second.get<string>("id");
                    LOG(debug) << "Found config for device id '" << deviceIdKey << "' in JSON input";
                }
            }
        }
    }
}

void DeviceParser(const boost::property_tree::ptree& tree, FairMQChannelMap& channelMap, const string& deviceId)
{
    string deviceIdKey;

    // For each node in fairMQOptions
    for (const auto& p : tree) {
        if (p.first == "devices") {
            for (const auto& q : p.second) {
                // check if key is provided, otherwise use id
                string key = q.second.get<string>("key", "");

                if (key != "") {
                    deviceIdKey = key;
                    // LOG(trace) << "Found config for device key '" << deviceIdKey << "' in JSON input";
                } else {
                    deviceIdKey = q.second.get<string>("id");
                    // LOG(trace) << "Found config for device id '" << deviceIdKey << "' in JSON input";
                }

                // if not correct device id, do not fill MQMap
                if (deviceId != deviceIdKey) {
                    continue;
                }

                LOG(trace) << "Found following channels for device ID '" << deviceId << "' :";

                ChannelParser(q.second, channelMap);
            }
        }
    }
}

void ChannelParser(const boost::property_tree::ptree& tree, FairMQChannelMap& channelMap)
{
    string channelKey;

    for (const auto& p : tree) {
        if (p.first == "channels") {
            for (const auto& q : p.second) {
                channelKey = q.second.get<string>("name");

                int numSockets = q.second.get<int>("numSockets", 0);

                // try to get common properties to use for all subChannels
                FairMQChannel commonChannel;
                commonChannel.UpdateType(q.second.get<string>("type", commonChannel.GetType()));
                commonChannel.UpdateMethod(q.second.get<string>("method", commonChannel.GetMethod()));
                commonChannel.UpdateAddress(q.second.get<string>("address", commonChannel.GetAddress()));
                commonChannel.UpdateTransport(q.second.get<string>("transport", commonChannel.GetTransportName()));
                commonChannel.UpdateSndBufSize(q.second.get<int>("sndBufSize", commonChannel.GetSndBufSize()));
                commonChannel.UpdateRcvBufSize(q.second.get<int>("rcvBufSize", commonChannel.GetRcvBufSize()));
                commonChannel.UpdateSndKernelSize(q.second.get<int>("sndKernelSize", commonChannel.GetSndKernelSize()));
                commonChannel.UpdateRcvKernelSize(q.second.get<int>("rcvKernelSize", commonChannel.GetRcvKernelSize()));
                commonChannel.UpdateLinger(q.second.get<int>("linger", commonChannel.GetLinger()));
                commonChannel.UpdateRateLogging(q.second.get<int>("rateLogging", commonChannel.GetRateLogging()));
                commonChannel.UpdatePortRangeMin(q.second.get<int>("portRangeMin", commonChannel.GetPortRangeMin()));
                commonChannel.UpdatePortRangeMax(q.second.get<int>("portRangeMax", commonChannel.GetPortRangeMax()));
                commonChannel.UpdateAutoBind(q.second.get<bool>("autoBind", commonChannel.GetAutoBind()));

                // temporary FairMQChannel container
                vector<FairMQChannel> channelList;

                if (numSockets > 0) {
                    LOG(trace) << "" << channelKey << ":";
                    LOG(trace) << "\tnumSockets of " << numSockets << " specified,";
                    LOG(trace) << "\tapplying common settings to each:";

                    LOG(trace) << "\ttype          = " << commonChannel.GetType();
                    LOG(trace) << "\tmethod        = " << commonChannel.GetMethod();
                    LOG(trace) << "\taddress       = " << commonChannel.GetAddress();
                    LOG(trace) << "\ttransport     = " << commonChannel.GetTransportName();
                    LOG(trace) << "\tsndBufSize    = " << commonChannel.GetSndBufSize();
                    LOG(trace) << "\trcvBufSize    = " << commonChannel.GetRcvBufSize();
                    LOG(trace) << "\tsndKernelSize = " << commonChannel.GetSndKernelSize();
                    LOG(trace) << "\trcvKernelSize = " << commonChannel.GetRcvKernelSize();
                    LOG(trace) << "\tlinger        = " << commonChannel.GetLinger();
                    LOG(trace) << "\trateLogging   = " << commonChannel.GetRateLogging();
                    LOG(trace) << "\tportRangeMin  = " << commonChannel.GetPortRangeMin();
                    LOG(trace) << "\tportRangeMax  = " << commonChannel.GetPortRangeMax();
                    LOG(trace) << "\tautoBind      = " << commonChannel.GetAutoBind();

                    for (int i = 0; i < numSockets; ++i) {
                        FairMQChannel channel(commonChannel);
                        channelList.push_back(channel);
                    }
                } else {
                    SocketParser(q.second.get_child(""), channelList, channelKey, commonChannel);
                }

                channelMap.insert(make_pair(channelKey, move(channelList)));
            }
        }
    }
}

void SocketParser(const boost::property_tree::ptree& tree, vector<FairMQChannel>& channelList, const string& channelName, const FairMQChannel& commonChannel)
{
    // for each socket in channel
    int socketCounter = 0;

    for (const auto& p : tree) {
        if (p.first == "sockets") {
            for (const auto& q : p.second) {
                // create new channel and apply setting from the common channel
                FairMQChannel channel(commonChannel);

                // if the socket field specifies or overrides something from the common channel, apply those settings
                channel.UpdateType(q.second.get<string>("type", channel.GetType()));
                channel.UpdateMethod(q.second.get<string>("method", channel.GetMethod()));
                channel.UpdateAddress(q.second.get<string>("address", channel.GetAddress()));
                channel.UpdateTransport(q.second.get<string>("transport", channel.GetTransportName()));
                channel.UpdateSndBufSize(q.second.get<int>("sndBufSize", channel.GetSndBufSize()));
                channel.UpdateRcvBufSize(q.second.get<int>("rcvBufSize", channel.GetRcvBufSize()));
                channel.UpdateSndKernelSize(q.second.get<int>("sndKernelSize", channel.GetSndKernelSize()));
                channel.UpdateRcvKernelSize(q.second.get<int>("rcvKernelSize", channel.GetRcvKernelSize()));
                channel.UpdateLinger(q.second.get<int>("linger", channel.GetLinger()));
                channel.UpdateRateLogging(q.second.get<int>("rateLogging", channel.GetRateLogging()));
                channel.UpdatePortRangeMin(q.second.get<int>("portRangeMin", channel.GetPortRangeMin()));
                channel.UpdatePortRangeMax(q.second.get<int>("portRangeMax", channel.GetPortRangeMax()));
                channel.UpdateAutoBind(q.second.get<bool>("autoBind", channel.GetAutoBind()));

                LOG(trace) << "" << channelName << "[" << socketCounter << "]:";
                LOG(trace) << "\ttype          = " << channel.GetType();
                LOG(trace) << "\tmethod        = " << channel.GetMethod();
                LOG(trace) << "\taddress       = " << channel.GetAddress();
                LOG(trace) << "\ttransport     = " << channel.GetTransportName();
                LOG(trace) << "\tsndBufSize    = " << channel.GetSndBufSize();
                LOG(trace) << "\trcvBufSize    = " << channel.GetRcvBufSize();
                LOG(trace) << "\tsndKernelSize = " << channel.GetSndKernelSize();
                LOG(trace) << "\trcvKernelSize = " << channel.GetRcvKernelSize();
                LOG(trace) << "\tlinger        = " << channel.GetLinger();
                LOG(trace) << "\trateLogging   = " << channel.GetRateLogging();
                LOG(trace) << "\tportRangeMin  = " << channel.GetPortRangeMin();
                LOG(trace) << "\tportRangeMax  = " << channel.GetPortRangeMax();
                LOG(trace) << "\tautoBind      = " << channel.GetAutoBind();

                channelList.push_back(channel);
                ++socketCounter;
            }
        }
    } // end socket loop

    if (socketCounter) {
        LOG(trace) << "Found " << socketCounter << " socket(s) in channel.";
    } else {
        LOG(trace) << "" << channelName << ":";
        LOG(trace) << "\tNo sockets specified,";
        LOG(trace) << "\tapplying common settings to the channel:";

        FairMQChannel channel(commonChannel);

        LOG(trace) << "\ttype          = " << channel.GetType();
        LOG(trace) << "\tmethod        = " << channel.GetMethod();
        LOG(trace) << "\taddress       = " << channel.GetAddress();
        LOG(trace) << "\ttransport     = " << channel.GetTransportName();
        LOG(trace) << "\tsndBufSize    = " << channel.GetSndBufSize();
        LOG(trace) << "\trcvBufSize    = " << channel.GetRcvBufSize();
        LOG(trace) << "\tsndKernelSize = " << channel.GetSndKernelSize();
        LOG(trace) << "\trcvKernelSize = " << channel.GetRcvKernelSize();
        LOG(trace) << "\tlinger        = " << channel.GetLinger();
        LOG(trace) << "\trateLogging   = " << channel.GetRateLogging();
        LOG(trace) << "\tportRangeMin  = " << channel.GetPortRangeMin();
        LOG(trace) << "\tportRangeMax  = " << channel.GetPortRangeMax();
        LOG(trace) << "\tautoBind      = " << channel.GetAutoBind();

        channelList.push_back(channel);
    }
}

} // Helper namespace

} // namespace parser
} // namespace mq
} // namespace fair

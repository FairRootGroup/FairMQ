/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
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
#include <boost/property_tree/xml_parser.hpp>

// WARNING : pragma commands to hide boost (1.54.0) warning
// TODO : remove these pragma commands when boost will fix this issue in future release
#if defined(__clang__)
    _Pragma("clang diagnostic push") 
    _Pragma("clang diagnostic ignored \"-Wshadow\"") 
    #include <boost/property_tree/json_parser.hpp> 
    _Pragma("clang diagnostic pop")
#elif defined(__GNUC__) || defined(__GNUG__)
    _Pragma("GCC diagnostic push")
    _Pragma("GCC diagnostic ignored \"-Wshadow\"")
    #include <boost/property_tree/json_parser.hpp> 
    _Pragma("GCC diagnostic pop")
#endif





namespace FairMQParser
{

// TODO : add key-value map<string,string> parameter  for replacing/updating values from keys
// function that convert property tree (given the xml or json structure) to FairMQMap
FairMQMap ptreeToMQMap(const boost::property_tree::ptree& pt, const std::string& deviceId, const std::string& rootNode, const std::string& formatFlag)
{
    // Create fair mq map
    FairMQMap channelMap;

    // variables to create key for the mq map. Note: maybe device name and id useless here
    std::string deviceIdKey;
    std::string channelKey;

    // do a first loop just to print the device-id in xml/json input
    for(const auto& p : pt.get_child(rootNode))
    {
        if (p.first != "device")
        {
            continue;
        }

        //get id attribute to choose the device
        if (formatFlag == "xml")
        {
            deviceIdKey = p.second.get<std::string>("<xmlattr>.id");
            LOG(DEBUG) << "Found device id '" << deviceIdKey << "' in XML input";
        }

        if (formatFlag == "json")
        {
            deviceIdKey = p.second.get<std::string>("id");
            LOG(DEBUG) << "Found device id '"<< deviceIdKey << "' in JSON input";
        }
    }

    // Extract value from boost::property_tree
    // For each device in fairMQOptions
    for(const auto& p : pt.get_child(rootNode))
    {
        if (p.first != "device")
        {
            continue;
        }

        //get id attribute to choose the device
        if (formatFlag == "xml")
        {
            deviceIdKey = p.second.get<std::string>("<xmlattr>.id");
        }

        if (formatFlag == "json")
        {
            deviceIdKey = p.second.get<std::string>("id");
        }

        // if not correct device id, do not fill MQMap
        if (deviceId != deviceIdKey)
        {
            continue;
        }

        // print if DEBUG log level set
        std::stringstream deviceStream;
        deviceStream << "[node = "     << p.first 
               << "]   id = "    << deviceIdKey;
        LOG(DEBUG) << deviceStream.str();

        // for each channel in device
        for(const auto& q : p.second.get_child(""))
        {
            if (q.first != "channel")
            {
                continue;
            }

            // get name attribute to form key
            if (formatFlag == "xml")
            {
                channelKey = q.second.get<std::string>("<xmlattr>.name");
            }

            if (formatFlag == "json")
            {
                channelKey = q.second.get<std::string>("name");
            }

            // print if DEBUG log level set
            std::stringstream channelStream;
            channelStream << "\t [node = " << q.first 
                    << "]   name = " << channelKey;
            LOG(DEBUG) << channelStream.str();

            // temporary FairMQChannel container
            std::vector<FairMQChannel> channelList;

            int socketCounter = 0;
            // for each socket in channel
            for (const auto& r : q.second.get_child(""))
            {
                if (r.first != "socket")
                {
                    continue;
                }

                ++socketCounter;
                FairMQChannel channel;

                // print if DEBUG log level set
                std::stringstream socket;
                socket << "\t \t [node = " << r.first 
                        << "]   socket index = " << socketCounter;
                LOG(DEBUG) << socket.str();
                LOG(DEBUG) <<  "\t \t \t type        = " << r.second.get<std::string>("type", channel.GetType());
                LOG(DEBUG) <<  "\t \t \t method      = " << r.second.get<std::string>("method", channel.GetMethod());
                LOG(DEBUG) <<  "\t \t \t address     = " << r.second.get<std::string>("address", channel.GetAddress());
                LOG(DEBUG) <<  "\t \t \t sndBufSize  = " << r.second.get<int>("sndBufSize", channel.GetSndBufSize());
                LOG(DEBUG) <<  "\t \t \t rcvBufSize  = " << r.second.get<int>("rcvBufSize", channel.GetRcvBufSize());
                LOG(DEBUG) <<  "\t \t \t rateLogging = " << r.second.get<int>("rateLogging", channel.GetRateLogging());

                channel.UpdateType(r.second.get<std::string>("type", channel.GetType()));
                channel.UpdateMethod(r.second.get<std::string>("method", channel.GetMethod()));
                channel.UpdateAddress(r.second.get<std::string>("address", channel.GetAddress()));
                channel.UpdateSndBufSize(r.second.get<int>("sndBufSize", channel.GetSndBufSize())); // int
                channel.UpdateRcvBufSize(r.second.get<int>("rcvBufSize", channel.GetRcvBufSize())); // int
                channel.UpdateRateLogging(r.second.get<int>("rateLogging", channel.GetRateLogging())); // int

                channelList.push_back(channel);
            }// end socket loop

            //fill mq map option
            channelMap.insert(std::make_pair(channelKey,std::move(channelList)));
        }
    }

    if (channelMap.size() > 0)
    {
        LOG(DEBUG) << "---- Channel-keys found are :";
        for (const auto& p : MQChannelMap)
        {
            LOG(DEBUG) << p.first;
        }
    }
    else
    {
        LOG(WARN) << "---- No channel-keys found for device-id " << deviceId;
        LOG(WARN) << "---- Check the "<< formatFlag << " inputs and/or command line inputs";
    }
    return channelMap;
}

////////////////////////////////////////////////////////////////////////////
FairMQMap JSON::UserParser(const std::string& filename, const std::string& deviceId, const std::string& rootNode)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(filename, pt);
    return ptreeToMQMap(pt, deviceId, rootNode,"json");
}

FairMQMap JSON::UserParser(std::stringstream& input, const std::string& deviceId, const std::string& rootNode)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(input, pt);
    return ptreeToMQMap(pt, deviceId, rootNode,"json");
}

////////////////////////////////////////////////////////////////////////////
FairMQMap XML::UserParser(const std::string& filename, const std::string& deviceId, const std::string& rootNode)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(filename, pt);
    return ptreeToMQMap(pt,deviceId,rootNode,"xml");
}

FairMQMap XML::UserParser(std::stringstream& input, const std::string& deviceId, const std::string& rootNode)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(input, pt);
    return ptreeToMQMap(pt,deviceId,rootNode,"xml");
}

} //  end FairMQParser namespace
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
#include <boost/property_tree/json_parser.hpp>

using namespace std;

namespace FairMQParser
{

// TODO : add key-value map<string,string> parameter  for replacing/updating values from keys
// function that convert property tree (given the xml or json structure) to FairMQMap
FairMQMap ptreeToMQMap(const boost::property_tree::ptree& pt, const string& deviceId, const string& rootNode, const string& formatFlag)
{
    // Create fair mq map
    FairMQMap channelMap;
    helper::PrintDeviceList(pt.get_child(rootNode));
    // Extract value from boost::property_tree
    helper::DeviceParser(pt.get_child(rootNode),channelMap,deviceId,formatFlag);
    if (channelMap.size() > 0)
    {
        LOG(DEBUG) << "---- Channel-keys found are :";
        for (const auto& p : channelMap)
        {
            LOG(DEBUG) << p.first;
        }
    }
    else
    {
        LOG(WARN) << "---- No channel-keys found for device-id " << deviceId;
        LOG(WARN) << "---- Check the JSON inputs and/or command line inputs";
    }

    return channelMap;
}


FairMQMap JSON::UserParser(const string& filename, const string& deviceId, const string& rootNode)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(filename, pt);
    return ptreeToMQMap(pt, deviceId, rootNode);
}

FairMQMap JSON::UserParser(stringstream& input, const string& deviceId, const string& rootNode)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_json(input, pt);
    return ptreeToMQMap(pt, deviceId, rootNode);
}

FairMQMap XML::UserParser(const string& filename, const string& deviceId, const string& rootNode)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(filename, pt);
    return ptreeToMQMap(pt, deviceId, rootNode, "xml");
}

FairMQMap XML::UserParser(stringstream& input, const string& deviceId, const string& rootNode)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(input, pt);
    return ptreeToMQMap(pt, deviceId, rootNode, "xml");
}




// /////////////////////////////////////////////////////////////////////////////////////////
// -----------------------------------------------------------------------------------------
namespace helper
{
    // -----------------------------------------------------------------------------------------

    void PrintDeviceList(const boost::property_tree::ptree& tree, const std::string& formatFlag)
    {
        string deviceIdKey;

        // do a first loop just to print the device-id in json input 
        for(const auto& p : tree)
        {
            if (p.first == "devices")
            {
                for(const auto& q : p.second.get_child(""))
                {
                    deviceIdKey = q.second.get<string>("id");
                    LOG(DEBUG) << "Found device id '" << deviceIdKey << "' in JSON input";
                }
            }
            
            if (p.first == "device")
            {
                //get id attribute to choose the device
                if (formatFlag == "xml")
                {
                    deviceIdKey = p.second.get<string>("<xmlattr>.id");
                    LOG(DEBUG) << "Found device id '" << deviceIdKey << "' in XML input";
                }

                if (formatFlag == "json")
                {
                    deviceIdKey = p.second.get<string>("id");
                    LOG(DEBUG) << "Found device id '"<< deviceIdKey << "' in JSON input";
                }
            }
            
        }
    }

    // -----------------------------------------------------------------------------------------

    void DeviceParser(const boost::property_tree::ptree& tree, FairMQMap& channelMap, const string& deviceId, const string& formatFlag)
    {
        string deviceIdKey;
        // For each node in fairMQOptions
        for(const auto& p0 : tree)
        {
            if (p0.first == "devices")
            {
                for(const auto& p : p0.second)
                {
                    deviceIdKey = p.second.get<string>("id");
                    LOG(TRACE) << "Found device id '"<< deviceIdKey << "' in JSON input";
                    
                    // if not correct device id, do not fill MQMap
                    if (deviceId != deviceIdKey)
                    {
                        continue;
                    }

                    // print if DEBUG log level set
                    stringstream deviceStream;
                    deviceStream << "[node = "     << p.first  << "]   id = "    << deviceIdKey;
                    LOG(DEBUG) << deviceStream.str();
                    helper::ChannelParser(p.second,channelMap,formatFlag);

                }
            }

            if (p0.first == "device")
            {
                
                if (formatFlag == "xml")
                {
                    deviceIdKey = p0.second.get<string>("<xmlattr>.id");
                    LOG(DEBUG) << "Found device id '" << deviceIdKey << "' in XML input";
                }

                if (formatFlag == "json")
                {
                    deviceIdKey = p0.second.get<string>("id");
                    LOG(DEBUG) << "Found device id '"<< deviceIdKey << "' in JSON input";
                }

                // if not correct device id, do not fill MQMap
                if (deviceId != deviceIdKey)
                {
                    continue;
                }

                

                // print if DEBUG log level set
                stringstream deviceStream;
                deviceStream << "[node = "     << p0.first  << "]   id = "    << deviceIdKey;
                LOG(DEBUG) << deviceStream.str();
                helper::ChannelParser(p0.second,channelMap,formatFlag);
            }
        }
    }

    // -----------------------------------------------------------------------------------------

    void ChannelParser(const boost::property_tree::ptree& tree, FairMQMap& channelMap, const string& formatFlag)
    {
        string channelKey;
        for(const auto& p : tree)
        {
            if(p.first=="channels")
            {
                for(const auto& q : p.second)
                {
                    channelKey = q.second.get<string>("name");
                    
                    // print if DEBUG log level set
                    stringstream channelStream;
                    channelStream << "\t [node = " << p.first  << "]   name = " << channelKey;
                    LOG(DEBUG) << channelStream.str();

                    // temporary FairMQChannel container
                    vector<FairMQChannel> channelList;
                    helper::SocketParser(q.second.get_child(""),channelList);
                    
                    //fill mq map option
                    channelMap.insert(make_pair(channelKey, move(channelList)));
                }
            }

            if(p.first=="channel")
            {
                
                // get name attribute to form key
                if (formatFlag == "xml")
                {
                    channelKey = p.second.get<string>("<xmlattr>.name");
                }

                if (formatFlag == "json")
                {
                    channelKey = p.second.get<string>("name");
                }

                stringstream channelStream;
                channelStream << "\t [node = " << p.first  << "]   name = " << channelKey;
                LOG(DEBUG) << channelStream.str();

                // temporary FairMQChannel container
                vector<FairMQChannel> channelList;
                helper::SocketParser(p.second.get_child(""),channelList);
                
                //fill mq map option
                channelMap.insert(make_pair(channelKey, move(channelList)));
            }
            
        }
    }

    // -----------------------------------------------------------------------------------------

    void SocketParser(const boost::property_tree::ptree& tree, vector<FairMQChannel>& channelList)
    {
        // for each socket in channel
        int socketCounter = 0;
        for (const auto& s : tree)
        {
            if (s.first == "sockets")
            {
                for (const auto& r : s.second)
                {
                    ++socketCounter;
                    FairMQChannel channel;

                    // print if DEBUG log level set
                    stringstream socket;
                    socket << "\t \t [node = " << s.first  << "]   socket index = " << socketCounter;
                    LOG(DEBUG) << socket.str();
                    LOG(DEBUG) <<  "\t \t \t type        = " << r.second.get<string>("type", channel.GetType());
                    LOG(DEBUG) <<  "\t \t \t method      = " << r.second.get<string>("method", channel.GetMethod());
                    LOG(DEBUG) <<  "\t \t \t address     = " << r.second.get<string>("address", channel.GetAddress());
                    LOG(DEBUG) <<  "\t \t \t sndBufSize  = " << r.second.get<int>("sndBufSize", channel.GetSndBufSize());
                    LOG(DEBUG) <<  "\t \t \t rcvBufSize  = " << r.second.get<int>("rcvBufSize", channel.GetRcvBufSize());
                    LOG(DEBUG) <<  "\t \t \t rateLogging = " << r.second.get<int>("rateLogging", channel.GetRateLogging());

                    channel.UpdateType(r.second.get<string>("type", channel.GetType()));
                    channel.UpdateMethod(r.second.get<string>("method", channel.GetMethod()));
                    channel.UpdateAddress(r.second.get<string>("address", channel.GetAddress()));
                    channel.UpdateSndBufSize(r.second.get<int>("sndBufSize", channel.GetSndBufSize())); // int
                    channel.UpdateRcvBufSize(r.second.get<int>("rcvBufSize", channel.GetRcvBufSize())); // int
                    channel.UpdateRateLogging(r.second.get<int>("rateLogging", channel.GetRateLogging())); // int

                    channelList.push_back(channel);
                }
            }

            if(s.first == "socket")
            {
                ++socketCounter;
                FairMQChannel channel;

                // print if DEBUG log level set
                stringstream socket;
                socket << "\t \t [node = " << s.first  << "]   socket index = " << socketCounter;
                LOG(DEBUG) << socket.str();
                LOG(DEBUG) <<  "\t \t \t type        = " << s.second.get<string>("type", channel.GetType());
                LOG(DEBUG) <<  "\t \t \t method      = " << s.second.get<string>("method", channel.GetMethod());
                LOG(DEBUG) <<  "\t \t \t address     = " << s.second.get<string>("address", channel.GetAddress());
                LOG(DEBUG) <<  "\t \t \t sndBufSize  = " << s.second.get<int>("sndBufSize", channel.GetSndBufSize());
                LOG(DEBUG) <<  "\t \t \t rcvBufSize  = " << s.second.get<int>("rcvBufSize", channel.GetRcvBufSize());
                LOG(DEBUG) <<  "\t \t \t rateLogging = " << s.second.get<int>("rateLogging", channel.GetRateLogging());

                channel.UpdateType(s.second.get<string>("type", channel.GetType()));
                channel.UpdateMethod(s.second.get<string>("method", channel.GetMethod()));
                channel.UpdateAddress(s.second.get<string>("address", channel.GetAddress()));
                channel.UpdateSndBufSize(s.second.get<int>("sndBufSize", channel.GetSndBufSize())); // int
                channel.UpdateRcvBufSize(s.second.get<int>("rcvBufSize", channel.GetRcvBufSize())); // int
                channel.UpdateRateLogging(s.second.get<int>("rateLogging", channel.GetRateLogging())); // int

                channelList.push_back(channel);
            }
            
        }// end socket loop
    }
} // end helper namespace

} //  end FairMQParser namespace

/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public License (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

/// @file   FairMQSuboptParser.cxx
/// @author Matthias.Richter@scieq.net
/// @since  2017-03-30
/// @brief  Parser implementation for key-value subopt format

#include "FairMQSuboptParser.h"
#include <boost/property_tree/ptree.hpp>
#include <cstring>

using boost::property_tree::ptree;

namespace FairMQParser
{

constexpr const char* SUBOPT::channelOptionKeys[];

FairMQMap SUBOPT::UserParser(const po::variables_map& omap, const std::string& deviceId, const std::string& rootNode)
{
    std::string nodeKey = rootNode + ".device";
    ptree pt;

    pt.put(nodeKey + ".id", deviceId.c_str());
    nodeKey += ".channels";

    // parsing of channel properties is the only implemented method right now
    if (omap.count(OptionKeyChannelConfig) > 0)
    {
        std::map<std::string, ptree> channelProperties;
        auto tokens = omap[OptionKeyChannelConfig].as<std::vector<std::string>>();
        for (auto token : tokens)
        {
            // std::map<std::string, ptree>::iterator channelProperty = channelProperties.end();
            ptree socketProperty;
            std::string channelName;
            std::string argString(token);
            char* subopts = &argString[0];
            char* value = nullptr;
            while (subopts && *subopts != 0 && *subopts != ' ')
            {
                // char* saved = subopts;
                int subopt=getsubopt(&subopts, (char**)channelOptionKeys, &value);
                if (subopt == NAME)
                {
                    channelName = value;
                    channelProperties[channelName].put("name", channelName);
                }
                else if (subopt>=0 && value != nullptr)
                {
                    socketProperty.put(channelOptionKeys[subopt], value);
                }
            }
            if (channelName != "")
            {
                channelProperties[channelName].add_child("sockets.socket", socketProperty);
            }
            else
            {
                // TODO: what is the error policy here, should we abort?
                LOG(ERROR) << "missing channel name in argument of option --channel-config";
            }
        }
        for (auto channelProperty : channelProperties)
        {
            pt.add_child(nodeKey + ".channel", channelProperty.second);
        }
    }

    return ptreeToMQMap(pt, deviceId, rootNode);
}

} // namespace FairMQParser

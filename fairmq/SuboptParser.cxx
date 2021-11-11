/********************************************************************************
 * Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public License (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

/// @file   SuboptParser.cxx
/// @author Matthias.Richter@scieq.net
/// @since  2017-03-30
/// @brief  Parser implementation for key-value subopt format

#include <fairmq/SuboptParser.h>
#include <fairmq/JSONParser.h>

#include <fairlogger/Logger.h>

#include <boost/property_tree/ptree.hpp>
#include <string_view>
#include <utility>   // make_pair
#include <cstring>

using boost::property_tree::ptree;
using namespace std;

namespace fair::mq
{

enum channelOptionKeyIds
{
    NAME = 0,       // name of the channel
    TYPE,           // push, pull, publish, subscribe, etc
    METHOD,         // bind or connect
    ADDRESS,        // host, protocol and port address
    TRANSPORT,      //
    SNDBUFSIZE,     // size of the send queue
    RCVBUFSIZE,     // size of the receive queue
    SNDKERNELSIZE,
    RCVKERNELSIZE,
    SNDTIMEOUTMS,
    RCVTIMEOUTMS,
    LINGER,
    RATELOGGING,    // logging rate
    PORTRANGEMIN,
    PORTRANGEMAX,
    AUTOBIND,
    NUMSOCKETS,
    lastsocketkey
};

constexpr static const char* channelOptionKeys[] = {
    /*[NAME]          = */ "name",
    /*[TYPE]          = */ "type",
    /*[METHOD]        = */ "method",
    /*[ADDRESS]       = */ "address",
    /*[TRANSPORT]     = */ "transport",
    /*[SNDBUFSIZE]    = */ "sndBufSize",
    /*[RCVBUFSIZE]    = */ "rcvBufSize",
    /*[SNDKERNELSIZE] = */ "sndKernelSize",
    /*[RCVKERNELSIZE] = */ "rcvKernelSize",
    /*[SNDTIMEOUTMS]  = */ "sndTimeoutMs",
    /*[RCVTIMEOUTMS]  = */ "rcvTimeoutMs",
    /*[LINGER]        = */ "linger",
    /*[RATELOGGING]   = */ "rateLogging",
    /*[PORTRANGEMIN]  = */ "portRangeMin",
    /*[PORTRANGEMAX]  = */ "portRangeMax",
    /*[AUTOBIND]      = */ "autoBind",
    /*[NUMSOCKETS]    = */ "numSockets",
    nullptr
};

Properties SuboptParser(const vector<string>& channelConfig, const string& deviceId)
{
    ptree pt;

    ptree devicesArray;
    ptree deviceProperties;

    ptree channelsArray;

    for (auto token : channelConfig) {
        string channelName;
        ptree channelProperties;

        ptree socketsArray;

        string argString(token);
        char* subopts = &argString[0];
        char* value = nullptr;
        // Find either a : or a =. If we find the former first, we consider what is before it
        // the channel name
        char* firstSep = strpbrk(subopts, ":=");
        if (firstSep && *firstSep == ':') {
            channelName = std::string_view(subopts, firstSep - subopts);
            channelProperties.put("name", channelName);
            subopts = firstSep + 1;
        }
        while (subopts && *subopts != 0 && *subopts != ' ') {
            char* cur = subopts;
            int subopt = getsubopt(&subopts, (char**)channelOptionKeys, &value);
            if (subopt == NAME) {
                channelName = value;
                channelProperties.put("name", channelName);
            } else if (subopt == ADDRESS) {
                ptree socketProperties;
                socketProperties.put(channelOptionKeys[subopt], value);
                socketsArray.push_back(make_pair("", socketProperties));
            } else if (subopt >= 0 && value != nullptr) {
                channelProperties.put(channelOptionKeys[subopt], value);
            } else if (subopt == -1) {
                LOG(warn) << "Ignoring unknown argument in --channel-config: " << cur;
            }
        }

        if (!channelName.empty()) {
            channelProperties.add_child("sockets", socketsArray);
        } else {
            // TODO: what is the error policy here, should we abort?
            LOG(error) << "missing channel name in argument of option --channel-config";
        }

        channelsArray.push_back(make_pair("", channelProperties));
    }

    deviceProperties.put("id", deviceId);
    deviceProperties.add_child("channels", channelsArray);

    devicesArray.push_back(make_pair("", deviceProperties));

    pt.add_child("fairMQOptions.devices", devicesArray);

    return PtreeParser(pt, deviceId);
}

}

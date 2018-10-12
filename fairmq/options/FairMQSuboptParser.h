/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public License (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

/// @file   FairMQSuboptParser.h
/// @author Matthias.Richter@scieq.net
/// @since  2017-03-30
/// @brief  Parser implementation for key-value subopt format

#ifndef FAIRMQPARSER_SUBOPT_H
#define FAIRMQPARSER_SUBOPT_H

#include "FairMQParser.h" // for FairMQChannelMap
#include <boost/program_options.hpp>
#include <cstring>
#include <vector>
#include <string>

namespace fair
{
namespace mq
{
namespace parser
{

/**
 * A parser implementation for FairMQ channel properties.
 * The parser handles a comma separated key=value list format by using the
 * getsubopt function of the standard library.
 *
 * The option key '--channel-config' can be used with the list of key/value
 * pairs like e.g.
 * <pre>
 * --channel-config name=output,type=push,method=bind
 * </pre>
 *
 * The FairMQ option parser defines a 'UserParser' function for different
 * formats. Currently it is strictly parsing channel options, but in general
 * the concept is extensible by renaming UserParser to ChannelPropertyParser
 * and introducing additional parser functions.
 */
struct SUBOPT
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
        LINGER,
        RATELOGGING,    // logging rate
        NUMSOCKETS,
        lastsocketkey
    };

    constexpr static const char *channelOptionKeys[] = {
        /*[NAME]          = */ "name",
        /*[TYPE]          = */ "type",
        /*[METHOD]        = */ "method",
        /*[ADDRESS]       = */ "address",
        /*[TRANSPORT]     = */ "transport",
        /*[SNDBUFSIZE]    = */ "sndBufSize",
        /*[RCVBUFSIZE]    = */ "rcvBufSize",
        /*[SNDKERNELSIZE] = */ "sndKernelSize",
        /*[RCVKERNELSIZE] = */ "rcvKernelSize",
        /*[LINGER]        = */ "linger",
        /*[RATELOGGING]   = */ "rateLogging",
        /*[NUMSOCKETS]    = */ "numSockets",
        nullptr
    };

    FairMQChannelMap UserParser(const std::vector<std::string>& channelConfig, const std::string& deviceId, const std::string& rootNode = "fairMQOptions");
};

}
}
}

#endif /* FAIRMQPARSER_SUBOPT_H */

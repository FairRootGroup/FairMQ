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

#ifndef FAIR_MQ_SUBOPTPARSER_H
#define FAIR_MQ_SUBOPTPARSER_H

#include <fairmq/Properties.h>

#include <vector>
#include <string>

namespace fair::mq
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

Properties SuboptParser(const std::vector<std::string>& channelConfig, const std::string& deviceId);

}

#endif /* FAIR_MQ_SUBOPTPARSER_H */

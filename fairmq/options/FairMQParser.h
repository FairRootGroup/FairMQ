/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/*
 * File:   FairMQParser.h
 * Author: winckler
 *
 * Created on May 14, 2015, 5:01 PM
 */

#ifndef FAIRMQPARSER_H
#define FAIRMQPARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <exception>

#include <boost/property_tree/ptree_fwd.hpp>

#include "FairMQChannel.h"
#include "Properties.h"

namespace fair
{
namespace mq
{
namespace parser
{

struct ParserError : std::runtime_error { using std::runtime_error::runtime_error; };

fair::mq::Properties ptreeToProperties(const boost::property_tree::ptree& pt, const std::string& deviceId);

struct JSON
{
    fair::mq::Properties UserParser(const std::string& filename, const std::string& deviceId);
};

namespace Helper
{

fair::mq::Properties DeviceParser(const boost::property_tree::ptree& tree, const std::string& deviceId);
void ChannelParser(const boost::property_tree::ptree& tree, fair::mq::Properties& properties);
void SubChannelParser(const boost::property_tree::ptree& tree, fair::mq::Properties& properties, const std::string& channelName, const fair::mq::Properties& commonProperties);

} // Helper namespace

} // namespace parser
} // namespace mq
} // namespace fair

#endif /* FAIRMQPARSER_H */

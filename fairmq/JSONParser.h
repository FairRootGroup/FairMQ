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

#ifndef FAIR_MQ_JSONPARSER_H
#define FAIR_MQ_JSONPARSER_H

#include <fairmq/Properties.h>
#include <boost/property_tree/ptree_fwd.hpp>

#include <stdexcept>
#include <string>

namespace fair
{
namespace mq
{

struct ParserError : std::runtime_error { using std::runtime_error::runtime_error; };

fair::mq::Properties PtreeParser(const boost::property_tree::ptree& pt, const std::string& deviceId);

fair::mq::Properties JSONParser(const std::string& filename, const std::string& deviceId);

namespace helper
{

fair::mq::Properties DeviceParser(const boost::property_tree::ptree& tree, const std::string& deviceId);
void ChannelParser(const boost::property_tree::ptree& tree, fair::mq::Properties& properties);
void SubChannelParser(const boost::property_tree::ptree& tree, fair::mq::Properties& properties, const std::string& channelName, const fair::mq::Properties& commonProperties);

} // helper namespace

} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_JSONPARSER_H */

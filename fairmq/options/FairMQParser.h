/* 
 * File:   FairMQParser.h
 * Author: winckler
 *
 * Created on May 14, 2015, 5:01 PM
 */

#ifndef FAIRMQPARSER_H
#define FAIRMQPARSER_H

// std
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

// Boost
#include <boost/property_tree/ptree.hpp>

// FairMQ
#include "FairMQChannel.h"

namespace FairMQParser
{

typedef std::unordered_map< std::string,std::vector<FairMQChannel> > FairMQMap;

FairMQMap ptreeToMQMap(const boost::property_tree::ptree& pt, const std::string& deviceId, const std::string& rootNode, const std::string& formatFlag = "json");

struct JSON
{
    FairMQMap UserParser(const std::string& filename, const std::string& deviceId, const std::string& rootNode = "fairMQOptions");
    FairMQMap UserParser(std::stringstream& input, const std::string& deviceId, const std::string& rootNode = "fairMQOptions");
};

struct XML
{
    FairMQMap UserParser(const std::string& filename, const std::string& deviceId, const std::string& root_node="fairMQOptions");
    FairMQMap UserParser(std::stringstream& input, const std::string& deviceId, const std::string& rootNode="fairMQOptions");
};

} // FairMQParser namespace
#endif /* FAIRMQPARSER_H */


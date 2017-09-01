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
#include <map>
#include <unordered_map>

#include <boost/property_tree/ptree.hpp>

#include "FairMQChannel.h"

namespace FairMQParser
{

using FairMQMap = std::unordered_map<std::string, std::vector<FairMQChannel>>;

FairMQMap ptreeToMQMap(const boost::property_tree::ptree& pt, const std::string& deviceId, const std::string& rootNode, const std::string& formatFlag = "json");

struct JSON
{
    FairMQMap UserParser(const std::string& filename, const std::string& deviceId, const std::string& rootNode = "fairMQOptions");
    FairMQMap UserParser(std::stringstream& input, const std::string& deviceId, const std::string& rootNode = "fairMQOptions");
};

struct XML
{
    FairMQMap UserParser(const std::string& filename, const std::string& deviceId, const std::string& rootNode = "fairMQOptions");
    FairMQMap UserParser(std::stringstream& input, const std::string& deviceId, const std::string& rootNode = "fairMQOptions");
};

namespace Helper
{

void PrintDeviceList(const boost::property_tree::ptree& tree, const std::string& formatFlag = "json");
void DeviceParser(const boost::property_tree::ptree& tree, FairMQMap& channelMap, const std::string& deviceId, const std::string& formatFlag);
void ChannelParser(const boost::property_tree::ptree& tree, FairMQMap& channelMap, const std::string& formatFlag);
void SocketParser(const boost::property_tree::ptree& tree, std::vector<FairMQChannel>& channelList, const std::string& channelName, const FairMQChannel& commonChannel);
void PrintPropertyTree(const boost::property_tree::ptree& tree, int level = 0);

} // Helper namespace

} // FairMQParser namespace

#endif /* FAIRMQPARSER_H */

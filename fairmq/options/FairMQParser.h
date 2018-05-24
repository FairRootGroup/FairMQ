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

namespace fair
{
namespace mq
{
namespace parser
{

using FairMQChannelMap = std::unordered_map<std::string, std::vector<FairMQChannel>>;

struct ParserError : std::runtime_error { using std::runtime_error::runtime_error; };

FairMQChannelMap ptreeToMQMap(const boost::property_tree::ptree& pt, const std::string& deviceId, const std::string& rootNode);

struct JSON
{
    FairMQChannelMap UserParser(const std::string& filename, const std::string& deviceId, const std::string& rootNode = "fairMQOptions");
};

namespace Helper
{

void PrintDeviceList(const boost::property_tree::ptree& tree);
void DeviceParser(const boost::property_tree::ptree& tree, FairMQChannelMap& channelMap, const std::string& deviceId);
void ChannelParser(const boost::property_tree::ptree& tree, FairMQChannelMap& channelMap);
void SocketParser(const boost::property_tree::ptree& tree, std::vector<FairMQChannel>& channelList, const std::string& channelName, const FairMQChannel& commonChannel);

} // Helper namespace

} // namespace parser
} // namespace mq
} // namespace fair

#endif /* FAIRMQPARSER_H */

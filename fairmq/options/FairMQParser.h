/* 
 * File:   FairMQParser.h
 * Author: winckler
 *
 * Created on May 14, 2015, 5:01 PM
 */

#ifndef FAIRMQPARSER_H
#define	FAIRMQPARSER_H

// FairRoot
#include "FairMQChannel.h"

// Boost
#include <boost/property_tree/ptree.hpp>

// std
#include <string>
#include <vector>
#include <map>


namespace FairMQParser
{
    
    typedef std::map<std::string, std::vector<FairMQChannel> > FairMQMap;
    FairMQMap ptreeToMQMap(const boost::property_tree::ptree& pt, const std::string& device_id, const std::string& root_node, const std::string& format_flag="json");

    struct JSON
    {
        FairMQMap UserParser(const std::string& filename, const std::string& device_id, const std::string& root_node="fairMQOptions");
        FairMQMap UserParser(std::stringstream& input_ss, const std::string& device_id, const std::string& root_node="fairMQOptions");
    };
    
    
} //  end FairMQParser namespace
#endif	/* FAIRMQPARSER_H */


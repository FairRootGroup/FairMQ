/* 
 * File:   FairMQParserExample.cxx
 * Author: winckler
 * 
 * Created on May 14, 2015, 5:01 PM
 */

#include "FairMQParserExample.h"
#include "FairMQLogger.h"
#include <boost/property_tree/xml_parser.hpp>


namespace FairMQParser
{

    // other xml examples
    ////////////////////////////////////////////////////////////////////////////
    boost::property_tree::ptree MQXML2::UserParser(const std::string& filename)
    {
        boost::property_tree::ptree pt;
        boost::property_tree::read_xml(filename, pt);
        return pt;
    }
    
    
    
    
    // TODO : finish implementation
    ////////////////////////////////////////////////////////////////////////////
    boost::property_tree::ptree MQXML3::UserParser(const std::string& filename, const std::string& devicename)
    {
        // Create an empty property tree object
        boost::property_tree::ptree pt;
        boost::property_tree::read_xml(filename, pt);
        
        
        // Create fair mq map
        
        auto xml =  pt.get_child("");
        std::vector<std::pair<std::string, boost::property_tree::ptree>> match;
        std::pair<std::string, boost::property_tree::ptree> device_match;
        
        ProcessTree(xml.begin (), xml.end (), std::back_inserter(match),
        [] (const std::string& key) { return key == "device"; });

        
        // for each device
        for (const auto& pair: match)
        {
            if(pair.second.get<std::string>("<xmlattr>.name") == devicename)
            {
                device_match=pair;
                
            }
            else
            {
                //match.erase(std::remove(match.begin(), match.end(), pair), match.end());
                continue;
            }
            
            //std::cout << "pair.first " << pair.first << std::endl;//device
            //std::cout   << "\t node = " << pair.first
            //            << "\t name = " << pair.second.get<std::string>("<xmlattr>.name")
            //            << "\t id = " << pair.second.get<std::string>("<xmlattr>.id");
            //std::cout<<std::endl;
        }
        
        return device_match.second;
    }
    
    
    
    
} //  end FairMQParser namespace

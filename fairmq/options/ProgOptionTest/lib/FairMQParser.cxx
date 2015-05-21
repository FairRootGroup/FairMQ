/* 
 * File:   FairMQParser.cxx
 * Author: winckler
 * 
 * Created on May 14, 2015, 5:01 PM
 */

#include "FairMQParser.h"
#include "FairMQLogger.h"
#include <boost/property_tree/xml_parser.hpp>

// WARNING : pragma commands to hide boost (1.54.0) warning
// TODO : remove these pragma commands when boost will fix this issue in future release
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#include <boost/property_tree/json_parser.hpp>
#pragma clang diagnostic pop

namespace FairMQParser
{
    no_id_exception NoIdErr;
    
    // TODO : add key-value map<string,string> parameter  for replacing/updating values from keys
    // function that convert property tree (given the xml or json structure) to FairMQMap
    FairMQMap boost_ptree_to_MQMap(const boost::property_tree::ptree& pt, const std::string& device_id, const std::string& root_node, const std::string& format_flag)
    {
        // Create fair mq map
        FairMQMap MQChannelMap;

        // variables to create key for the mq map. Note: maybe device name and id useless here
        std::string kdevice_id;
        std::string kchannel;

        if(device_id.empty())
            throw NoIdErr;
        
        
        // do a first loop just to print the device-id in xml/json input
        for(const auto& p : pt.get_child(root_node))
        {
            if (p.first != "device")
                continue;

            //get id attribute to choose the device
            if(format_flag=="xml")
            {
                kdevice_id=p.second.get<std::string>("<xmlattr>.id");
                MQLOG(DEBUG)<<"Found device id '"<< kdevice_id <<"' in XML input";
            }
            
            if(format_flag=="json")
            {
                kdevice_id=p.second.get<std::string>("id");
                MQLOG(DEBUG)<<"Found device id '"<< kdevice_id <<"' in JSON input";
            }
        }
        
        // Extract value from boost::property_tree
        // For each device in fairMQOptions
        for(const auto& p : pt.get_child(root_node))
        {
            if (p.first != "device")
                continue;

            //get id attribute to choose the device
            if(format_flag=="xml")
                kdevice_id=p.second.get<std::string>("<xmlattr>.id");
            
            if(format_flag=="json")
                kdevice_id=p.second.get<std::string>("id");
            // if not correct device id, do not fill MQMap
            if(device_id != kdevice_id)
                continue;
            
            // print if DEBUG log level set
            std::stringstream ss_device;
            ss_device << "[node = "     << p.first 
                      << "]   id = "     << kdevice_id;
            MQLOG(DEBUG)<<ss_device.str();

            // for each channel in device
            for(const auto& q : p.second.get_child(""))
            {
                if (q.first != "channel")
                    continue;
                
                //get name attribute to form key
                if(format_flag=="xml")
                    kchannel=q.second.get<std::string>("<xmlattr>.name");
                
                if(format_flag=="json")
                    kchannel=q.second.get<std::string>("name");
                
                // print if DEBUG log level set
                std::stringstream ss_chan;
                ss_chan << "\t [node = " << q.first 
                        << "]   name = " << kchannel;
                MQLOG(DEBUG)<<ss_chan.str();

                // temporary FairMQChannel container
                std::vector<FairMQChannel> channel_list;
                
                int count_socket=0;
                // for each socket in channel
                for(const auto& r : q.second.get_child(""))
                {
                    if (r.first != "socket")
                        continue;
                    
                    count_socket++;
                    FairMQChannel channel;
                    
                    // print if DEBUG log level set
                    std::stringstream ss_sock;
                    ss_sock << "\t \t [node = " << r.first 
                            << "]   socket index = " << count_socket;
                    MQLOG(DEBUG)<<ss_sock.str();
                    MQLOG(DEBUG)<< "\t \t \t type        = " << r.second.get<std::string>("type",channel.fType);
                    MQLOG(DEBUG)<< "\t \t \t method      = " << r.second.get<std::string>("method",channel.fMethod);
                    MQLOG(DEBUG)<< "\t \t \t address     = " << r.second.get<std::string>("address",channel.fAddress);
                    MQLOG(DEBUG)<< "\t \t \t sndBufSize  = " << r.second.get<int>("sndBufSize",channel.fSndBufSize);
                    MQLOG(DEBUG)<< "\t \t \t rcvBufSize  = " << r.second.get<int>("rcvBufSize",channel.fRcvBufSize);
                    MQLOG(DEBUG)<< "\t \t \t rateLogging = " << r.second.get<int>("rateLogging",channel.fRateLogging);

                    
                    channel.fType        = r.second.get<std::string>("type",channel.fType);
                    channel.fMethod      = r.second.get<std::string>("method",channel.fMethod);
                    channel.fAddress     = r.second.get<std::string>("address",channel.fAddress);
                    channel.fSndBufSize  = r.second.get<int>("sndBufSize",channel.fSndBufSize);//int
                    channel.fRcvBufSize  = r.second.get<int>("rcvBufSize",channel.fRcvBufSize);//int
                    channel.fRateLogging = r.second.get<int>("rateLogging",channel.fRateLogging);//int

                    channel_list.push_back(channel);
                }// end socket loop
                
                //fill mq map option
                MQChannelMap.insert(std::make_pair(kchannel,std::move(channel_list)));
            }
        }
        
        if(MQChannelMap.size()>0)
        {
            MQLOG(DEBUG)<<"---- Channel-keys found are :";
            for(const auto& p : MQChannelMap)
                MQLOG(DEBUG)<<p.first;
        }
        else
        {
            MQLOG(WARN)<<"---- No channel-keys found for device-id "<<device_id;
            MQLOG(WARN)<<"---- Check the "<< format_flag <<" inputs and/or command line inputs";
        }
        return MQChannelMap;
    }
    
    
    
    
    
    ////////////////////////////////////////////////////////////////////////////
    //----------- filename version
    FairMQMap XML::UserParser(const std::string& filename, const std::string& device_id, const std::string& root_node)
    {
        boost::property_tree::ptree pt;
        boost::property_tree::read_xml(filename, pt);
        return boost_ptree_to_MQMap(pt,device_id,root_node,"xml");
    }
    
    
    
    //----------- stringstream version
    FairMQMap XML::UserParser(std::stringstream& input_ss, const std::string& device_id, const std::string& root_node)
    {
        boost::property_tree::ptree pt;
        boost::property_tree::read_xml(input_ss, pt);
        return boost_ptree_to_MQMap(pt,device_id,root_node,"xml");
    }
    
    
    ////////////////////////////////////////////////////////////////////////////
    FairMQMap JSON::UserParser(const std::string& filename, const std::string& device_id, const std::string& root_node)
    {
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(filename, pt);
        return boost_ptree_to_MQMap(pt,device_id,root_node,"json");
    }
    
    FairMQMap JSON::UserParser(std::stringstream& input_ss, const std::string& device_id, const std::string& root_node)
    {
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(input_ss, pt);
        return boost_ptree_to_MQMap(pt,device_id,root_node,"json");
    }
    
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
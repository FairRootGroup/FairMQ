/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/*
 * File:   FairMQProgOptions.cxx
 * Author: winckler
 * 
 * Created on March 11, 2015, 10:20 PM
 */

#include "FairMQProgOptions.h"
#include <algorithm>
#include "FairMQParser.h"
using namespace std;

FairMQProgOptions::FairMQProgOptions()
    : FairProgOptions()
    , fMQParserOptions("MQ-Device parser options")
    , fMQOptionsInCfg("MQ-Device options")
    , fMQOptionsInCmd("MQ-Device options")
    , fFairMQMap()
    , fHelpTitle("***** FAIRMQ Program Options ***** ")
    , fVersion("Beta version 0.1")
    , fMQKeyMap()
{
}

// ----------------------------------------------------------------------------------

FairMQProgOptions::~FairMQProgOptions()
{
}

// ----------------------------------------------------------------------------------

void FairMQProgOptions::ParseAll(const int argc, char** argv, bool allowUnregistered)
{
    
    // init description
    InitOptionDescription();
    // parse command line options
    if (ParseCmdLine(argc, argv, fCmdLineOptions, fVarMap, allowUnregistered))
    {
        LOG(ERROR) << "Could not parse cmd options";
        exit(EXIT_FAILURE);
    }

    // if txt/INI configuration file enabled then parse it as well
    if (fUseConfigFile)
    {
        // check if file exist
        if (fs::exists(fConfigFile))
        {
            if (ParseCfgFile(fConfigFile.string(), fConfigFileOptions, fVarMap, allowUnregistered))
            {
                LOG(ERROR) << "Could not parse config";
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            LOG(ERROR) << "config file '" << fConfigFile << "' not found";
            exit(EXIT_FAILURE);
        }
    }

    // set log level before printing (default is 0 = DEBUG level)
    std::string verbosity = GetValue<std::string>("verbose");
    bool color = GetValue<bool>("log-color");
    if (!color)
    {
        reinit_logger(false);
    }

    if (fSeverityMap.count(verbosity))
    {
        set_global_log_level(log_op::operation::GREATER_EQ_THAN, fSeverityMap.at(verbosity));
    }
    else
    {
        LOG(ERROR) << " verbosity level '" << verbosity << "' unknown, it will be set to DEBUG";
        set_global_log_level(log_op::operation::GREATER_EQ_THAN, fSeverityMap.at("DEBUG"));
    }

    PrintOptions();

    // check if one of required MQ config option is there  
    auto parserOption_shptr = fMQParserOptions.options();
    bool optionExists = false;
    vector<string> MQParserKeys;
    for (const auto& p : parserOption_shptr)
    {
        MQParserKeys.push_back(p->canonical_display_name());
        if (fVarMap.count(p->canonical_display_name()))
        {
            optionExists = true;
            break;
        }
    }

    if (!optionExists)
    {
        LOG(WARN) << "Options to configure FairMQ channels are not provided.";
        LOG(WARN) << "Please provide the value for one of the following keys:";
        for (const auto& p : MQParserKeys)
        {
            LOG(WARN) << p;
        }
        LOG(WARN) << "No channels will be created (You can create them manually).";
        // return 1;
    }
    else
    {
        if (fVarMap.count("mq-config"))
        {
            LOG(DEBUG) << "mq-config: Using default XML/JSON parser";

            std::string file = fVarMap["mq-config"].as<std::string>();
            std::string id;

            if (fVarMap.count("config-key"))
            {
                id = fVarMap["config-key"].as<std::string>();
            }
            else
            {
                id = fVarMap["id"].as<std::string>();
            }

            std::string fileExtension = boost::filesystem::extension(file);

            std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

            if (fileExtension == ".json")
            {
                UserParser<FairMQParser::JSON>(file, id);
            }
            else
            {
                if (fileExtension == ".xml")
                {
                    UserParser<FairMQParser::XML>(file, id);
                }
                else
                {
                    LOG(ERROR)  << "mq-config command line called but file extension '"
                                << fileExtension
                                << "' not recognized. Program will now exit";
                    exit(EXIT_FAILURE);
                }
            }
        }
        else if (fVarMap.count("config-json-string"))
        {
            LOG(DEBUG) << "config-json-string: Parsing JSON string";

            std::string id = fVarMap["id"].as<std::string>();

            std::string value = fairmq::ConvertVariableValue<fairmq::ToString>().Run(fVarMap.at("config-json-string"));
            std::stringstream ss;
            ss << value;
            UserParser<FairMQParser::JSON>(ss, id);
        }
        else if (fVarMap.count("config-xml-string"))
        {
            LOG(DEBUG) << "config-json-string: Parsing XML string";

            std::string id = fVarMap["id"].as<std::string>();

            std::string value = fairmq::ConvertVariableValue<fairmq::ToString>().Run(fVarMap.at("config-xml-string"));
            std::stringstream ss;
            ss << value;
            UserParser<FairMQParser::XML>(ss, id);
        }
    }
}


// ----------------------------------------------------------------------------------

int FairMQProgOptions::Store(const FairMQMap& channels)
{
    fFairMQMap = channels;
    UpdateMQValues();
    return 0;
}


// ----------------------------------------------------------------------------------

// replace FairMQChannelMap, and update variable map accordingly
int FairMQProgOptions::UpdateChannelMap(const FairMQMap& channels)
{
    fFairMQMap=channels;
    UpdateMQValues();
    return 0;
}

// ----------------------------------------------------------------------------------

// read FairMQChannelMap and insert/update corresponding values in variable map
// create key for variable map as follow : channelName.index.memberName
void FairMQProgOptions::UpdateMQValues()
{

    for(const auto& p : fFairMQMap)
    {
        int index = 0;
        for(const auto& channel : p.second)
        {
            std::string typeKey = p.first + "." + std::to_string(index) + ".type";
            std::string methodKey = p.first + "." + std::to_string(index) + ".method";
            std::string addressKey = p.first + "." + std::to_string(index) + ".address";
            std::string propertyKey = p.first + "." + std::to_string(index) + ".property";
            std::string sndBufSizeKey = p.first + "." + std::to_string(index) + ".sndBufSize";
            std::string rcvBufSizeKey = p.first + "." + std::to_string(index) + ".rcvBufSize";
            std::string rateLoggingKey = p.first + "." + std::to_string(index) + ".rateLogging";
            
            fMQKeyMap[typeKey] = std::make_tuple(p.first,index,"type");
            fMQKeyMap[methodKey] = std::make_tuple(p.first,index,"method");
            fMQKeyMap[addressKey] = std::make_tuple(p.first,index,"address");
            fMQKeyMap[propertyKey] = std::make_tuple(p.first,index,"property");
            fMQKeyMap[sndBufSizeKey] = std::make_tuple(p.first,index,"sndBufSize");
            fMQKeyMap[rcvBufSizeKey] = std::make_tuple(p.first,index,"rcvBufSize");
            fMQKeyMap[rateLoggingKey] = std::make_tuple(p.first,index,"rateLogging");
            
            UpdateVarMap<std::string>(typeKey,channel.GetType());
            UpdateVarMap<std::string>(methodKey,channel.GetMethod());
            UpdateVarMap<std::string>(addressKey,channel.GetAddress());
            UpdateVarMap<std::string>(propertyKey,channel.GetProperty());
            UpdateVarMap<int>(sndBufSizeKey,channel.GetSndBufSize());
            UpdateVarMap<int>(rcvBufSizeKey,channel.GetRcvBufSize());
            UpdateVarMap<int>(rateLoggingKey,channel.GetRateLogging());

            /*
            LOG(DEBUG) << "Update MQ parameters of variable map";
            LOG(DEBUG) << "key = " << typeKey <<"\t value = " << GetValue<std::string>(typeKey);
            LOG(DEBUG) << "key = " << methodKey <<"\t value = " << GetValue<std::string>(methodKey);
            LOG(DEBUG) << "key = " << addressKey <<"\t value = " << GetValue<std::string>(addressKey);
            LOG(DEBUG) << "key = " << propertyKey <<"\t value = " << GetValue<std::string>(propertyKey);
            LOG(DEBUG) << "key = " << sndBufSizeKey << "\t value = " << GetValue<int>(sndBufSizeKey);
            LOG(DEBUG) << "key = " << rcvBufSizeKey <<"\t value = " << GetValue<int>(rcvBufSizeKey);
            LOG(DEBUG) << "key = " << rateLoggingKey <<"\t value = " << GetValue<int>(rateLoggingKey);
            */
            index++;
        }

    }

}

// ----------------------------------------------------------------------------------


int FairMQProgOptions::NotifySwitchOption()
{
    if (fVarMap.count("help"))
    {
        LOG(INFO) << fHelpTitle << "\n" << fVisibleOptions;
        return 1;
    }

    if (fVarMap.count("version")) 
    {
        LOG(INFO) << fVersion << "\n";
        return 1;
    }

    return 0;
}

// ----------------------------------------------------------------------------------

void FairMQProgOptions::InitOptionDescription()
{
    // Id required in command line if config txt file not enabled
    if (fUseConfigFile)
    {
        fMQOptionsInCmd.add_options()
            ("id",                po::value<string>(),                               "Device ID (required argument).")
            ("io-threads",        po::value<int   >()->default_value(1),             "Number of I/O threads.")
            ("transport",         po::value<string>()->default_value("zeromq"),      "Transport ('zeromq'/'nanomsg').")
            ("deployment",        po::value<string>()->default_value("static"),      "Deployment ('static'/'dds').")
            ("control",           po::value<string>()->default_value("interactive"), "States control ('interactive'/'static'/'dds').")
            ("network-interface", po::value<string>()->default_value("eth0"),        "Network interface to bind on (e.g. eth0, ib0, wlan0, en0, lo...).")
            ("config-key",        po::value<string>(),                               "Use provided value instead of device id for fetching the configuration from the config file")
            ;

        fMQOptionsInCfg.add_options()
            ("id",                po::value<string>()->required(),                   "Device ID (required argument).")
            ("io-threads",        po::value<int   >()->default_value(1),             "Number of I/O threads.")
            ("transport",         po::value<string>()->default_value("zeromq"),      "Transport ('zeromq'/'nanomsg').")
            ("deployment",        po::value<string>()->default_value("static"),      "Deployment ('static'/'dds').")
            ("control",           po::value<string>()->default_value("interactive"), "States control ('interactive'/'static'/'dds').")
            ("network-interface", po::value<string>()->default_value("eth0"),        "Network interface to bind on (e.g. eth0, ib0, wlan0, en0, lo...).")
            ("config-key",        po::value<string>(),                               "Use provided value instead of device id for fetching the configuration from the config file")
            ;
    }
    else
    {
        fMQOptionsInCmd.add_options()
            ("id",                po::value<string>()->required(),                   "Device ID (required argument)")
            ("io-threads",        po::value<int   >()->default_value(1),             "Number of I/O threads")
            ("transport",         po::value<string>()->default_value("zeromq"),      "Transport ('zeromq'/'nanomsg').")
            ("deployment",        po::value<string>()->default_value("static"),      "Deployment ('static'/'dds').")
            ("control",           po::value<string>()->default_value("interactive"), "States control ('interactive'/'static'/'dds').")
            ("network-interface", po::value<string>()->default_value("eth0"),        "Network interface to bind on (e.g. eth0, ib0, wlan0, en0, lo...).")
            ("config-key",        po::value<string>(),                               "Use provided value instead of device id for fetching the configuration from the config file")
            ;
    }

    fMQParserOptions.add_options()
        ("config-xml-string",  po::value<vector<string>>()->multitoken(), "XML input as command line string.")
        // ("config-xml-file",    po::value<string>(),                       "XML input as file.")
        ("config-json-string", po::value<vector<string>>()->multitoken(), "JSON input as command line string.")
        // ("config-json-file",   po::value<string>(),                       "JSON input as file.")
        ("mq-config",          po::value<string>(),                       "JSON/XML input as file. The configuration object will check xml or json file extention and will call the json or xml parser accordingly")
        ;

    AddToCmdLineOptions(fGenericDesc);
    AddToCmdLineOptions(fMQOptionsInCmd);
    AddToCmdLineOptions(fMQParserOptions);

    if (fUseConfigFile)
    {
        AddToCfgFileOptions(fMQOptionsInCfg, false);
        AddToCfgFileOptions(fMQParserOptions, false);
    }
}

// ----------------------------------------------------------------------------------

int FairMQProgOptions::UpdateChannelMap(const std::string& channelName, int index, const std::string& member, const std::string& val)
{
    if(member == "type") 
    {
        fFairMQMap.at(channelName).at(index).UpdateType(val);
        return 0;
    }
    
    if(member == "method") 
    {
        fFairMQMap.at(channelName).at(index).UpdateMethod(val);
        return 0;
    }
    
    if(member == "address") 
    {
        fFairMQMap.at(channelName).at(index).UpdateAddress(val);
        return 0;
    }
    
    if(member == "property") 
    {
        fFairMQMap.at(channelName).at(index).UpdateProperty(val);
        return 0;
    }
    else
    {
        //if we get there it means something is wrong
        LOG(ERROR)  << "update of FairMQChannel map failed for the following key: "
                    << channelName<<"."<<index<<"."<<member;
        return 1;
    }

}

// ----------------------------------------------------------------------------------


int FairMQProgOptions::UpdateChannelMap(const std::string& channelName, int index, const std::string& member, int val)
{
    
    if(member == "sndBufSize") 
    {
        fFairMQMap.at(channelName).at(index).UpdateSndBufSize(val);
        return 0;
    }
    
    if(member == "rcvBufSize") 
    {
        fFairMQMap.at(channelName).at(index).UpdateRcvBufSize(val);
        return 0;
    }

    if(member == "rateLogging") 
    {
        fFairMQMap.at(channelName).at(index).UpdateRateLogging(val);
        return 0;
    }
    else
    {
        //if we get there it means something is wrong
        LOG(ERROR)  << "update of FairMQChannel map failed for the following key: "
                    << channelName<<"."<<index<<"."<<member;
        return 1;
    }
}

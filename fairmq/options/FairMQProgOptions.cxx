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
#include "FairMQLogger.h"

using namespace std;

FairMQProgOptions::FairMQProgOptions()
    : FairProgOptions(), FairMQEventManager()
    , fMQParserOptions("MQ-Device parser options")
    , fMQOptionsInCfg("MQ-Device options")
    , fMQOptionsInCmd("MQ-Device options")
    , fFairMQMap()
    , fHelpTitle("***** FAIRMQ Program Options ***** ")
    , fVersion("Beta version 0.1")
    , fMQKeyMap()
    // , fSignalMap() //string API
{
}

FairMQProgOptions::~FairMQProgOptions()
{
}

void FairMQProgOptions::ParseAll(const int argc, char** argv, bool allowUnregistered)
{
    // init description
    InitOptionDescription();
    // parse command line options
    if (FairProgOptions::ParseCmdLine(argc, argv, fCmdLineOptions, fVarMap, allowUnregistered))
    {
        // ParseCmdLine return 0 if help or version cmd not called. return 1 if called
        exit(EXIT_SUCCESS);
    }

    // if txt/INI configuration file enabled then parse it as well
    if (fUseConfigFile)
    {
        // check if file exist
        if (fs::exists(fConfigFile))
        {
            if (FairProgOptions::ParseCfgFile(fConfigFile.string(), fConfigFileOptions, fVarMap, allowUnregistered))
            {
                // ParseCfgFile return -1 if cannot open or read config file. It return 0 otherwise
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

    string verbosity = GetValue<string>("verbosity");
    string logFile = GetValue<string>("log-to-file");
    bool color = GetValue<bool>("log-color");

    // check if the provided verbosity level is valid, otherwise set to DEBUG
    if (fSeverityMap.count(verbosity) == 0)
    {
        LOG(ERROR) << " verbosity level '" << verbosity << "' unknown, it will be set to DEBUG";
        verbosity = "DEBUG";
    }

    if (logFile != "")
    {
        reinit_logger(false, logFile, fSeverityMap.at(verbosity));
        DefaultConsoleSetFilter(fSeverityMap.at("NOLOG"));
    }
    else
    {
        if (!color)
        {
            reinit_logger(false);
        }

        DefaultConsoleSetFilter(fSeverityMap.at(verbosity));
    }

    // check if one of required MQ config option is there  
    auto parserOptions = fMQParserOptions.options();
    bool optionExists = false;
    vector<string> MQParserKeys;
    for (const auto& p : parserOptions)
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
        // if cmdline mq-config called then use the default xml/json parser
        if (fVarMap.count("mq-config"))
        {
            LOG(DEBUG) << "mq-config: Using default XML/JSON parser";

            string file = fVarMap["mq-config"].as<string>();
            string id;

            if (fVarMap.count("config-key"))
            {
                id = fVarMap["config-key"].as<string>();
            }
            else
            {
                id = fVarMap["id"].as<string>();
            }

            string fileExtension = boost::filesystem::extension(file);

            transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);

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

            string id;

            if (fVarMap.count("config-key"))
            {
                id = fVarMap["config-key"].as<string>();
            }
            else
            {
                id = fVarMap["id"].as<string>();
            }

            string value = FairMQ::ConvertVariableValue<FairMQ::ToString>().Run(fVarMap.at("config-json-string"));
            stringstream ss;
            ss << value;
            UserParser<FairMQParser::JSON>(ss, id);
        }
        else if (fVarMap.count("config-xml-string"))
        {
            LOG(DEBUG) << "config-json-string: Parsing XML string";

            string id;

            if (fVarMap.count("config-key"))
            {
                id = fVarMap["config-key"].as<string>();
            }
            else
            {
                id = fVarMap["id"].as<string>();
            }

            string value = FairMQ::ConvertVariableValue<FairMQ::ToString>().Run(fVarMap.at("config-xml-string"));
            stringstream ss;
            ss << value;
            UserParser<FairMQParser::XML>(ss, id);
        }
    }

    FairProgOptions::PrintOptions();
}

int FairMQProgOptions::Store(const FairMQMap& channels)
{
    fFairMQMap = channels;
    UpdateMQValues();
    return 0;
}

// replace FairMQChannelMap, and update variable map accordingly
int FairMQProgOptions::UpdateChannelMap(const FairMQMap& channels)
{
    fFairMQMap = channels;
    UpdateMQValues();
    return 0;
}

// read FairMQChannelMap and insert/update corresponding values in variable map
// create key for variable map as follow : channelName.index.memberName
void FairMQProgOptions::UpdateMQValues()
{
    for (const auto& p : fFairMQMap)
    {
        int index = 0;

        for (const auto& channel : p.second)
        {
            string typeKey = p.first + "." + to_string(index) + ".type";
            string methodKey = p.first + "." + to_string(index) + ".method";
            string addressKey = p.first + "." + to_string(index) + ".address";
            string sndBufSizeKey = p.first + "." + to_string(index) + ".sndBufSize";
            string rcvBufSizeKey = p.first + "." + to_string(index) + ".rcvBufSize";
            string sndKernelSizeKey = p.first + "." + to_string(index) + ".sndKernelSize";
            string rcvKernelSizeKey = p.first + "." + to_string(index) + ".rcvKernelSize";
            string rateLoggingKey = p.first + "." + to_string(index) + ".rateLogging";

            fMQKeyMap[typeKey] = make_tuple(p.first, index, "type");
            fMQKeyMap[methodKey] = make_tuple(p.first, index, "method");
            fMQKeyMap[addressKey] = make_tuple(p.first, index, "address");
            fMQKeyMap[sndBufSizeKey] = make_tuple(p.first, index, "sndBufSize");
            fMQKeyMap[rcvBufSizeKey] = make_tuple(p.first, index, "rcvBufSize");
            fMQKeyMap[sndKernelSizeKey] = make_tuple(p.first, index, "sndKernelSize");
            fMQKeyMap[rcvKernelSizeKey] = make_tuple(p.first, index, "rcvkernelSize");
            fMQKeyMap[rateLoggingKey] = make_tuple(p.first, index, "rateLogging");

            UpdateVarMap<string>(typeKey, channel.GetType());
            UpdateVarMap<string>(methodKey, channel.GetMethod());
            UpdateVarMap<string>(addressKey, channel.GetAddress());


            //UpdateVarMap<string>(sndBufSizeKey, to_string(channel.GetSndBufSize()));// string API
            UpdateVarMap<int>(sndBufSizeKey, channel.GetSndBufSize());

            //UpdateVarMap<string>(rcvBufSizeKey, to_string(channel.GetRcvBufSize()));// string API
            UpdateVarMap<int>(rcvBufSizeKey, channel.GetRcvBufSize());

            //UpdateVarMap<string>(sndKernelSizeKey, to_string(channel.GetSndKernelSize()));// string API
            UpdateVarMap<int>(sndKernelSizeKey, channel.GetSndKernelSize());

            //UpdateVarMap<string>(rcvKernelSizeKey, to_string(channel.GetRcvKernelSize()));// string API
            UpdateVarMap<int>(rcvKernelSizeKey, channel.GetRcvKernelSize());

            //UpdateVarMap<string>(rateLoggingKey,to_string(channel.GetRateLogging()));// string API
            UpdateVarMap<int>(rateLoggingKey, channel.GetRateLogging());

            /*
            LOG(DEBUG) << "Update MQ parameters of variable map";
            LOG(DEBUG) << "key = " << typeKey <<"\t value = " << GetValue<string>(typeKey);
            LOG(DEBUG) << "key = " << methodKey <<"\t value = " << GetValue<string>(methodKey);
            LOG(DEBUG) << "key = " << addressKey <<"\t value = " << GetValue<string>(addressKey);
            LOG(DEBUG) << "key = " << sndBufSizeKey << "\t value = " << GetValue<int>(sndBufSizeKey);
            LOG(DEBUG) << "key = " << rcvBufSizeKey <<"\t value = " << GetValue<int>(rcvBufSizeKey);
            LOG(DEBUG) << "key = " << sndKernelSizeKey << "\t value = " << GetValue<int>(sndKernelSizeKey);
            LOG(DEBUG) << "key = " << rcvKernelSizeKey <<"\t value = " << GetValue<int>(rcvKernelSizeKey);
            LOG(DEBUG) << "key = " << rateLoggingKey <<"\t value = " << GetValue<int>(rateLoggingKey);
            */
            index++;
        }
        UpdateVarMap<int>(p.first + ".numSockets", index);
    }
}

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

void FairMQProgOptions::InitOptionDescription()
{
    // Id required in command line if config txt file not enabled
    if (fUseConfigFile)
    {
        fMQOptionsInCmd.add_options()
            ("id",                po::value<string>(),                               "Device ID (required argument).")
            ("io-threads",        po::value<int   >()->default_value(1),             "Number of I/O threads.")
            ("transport",         po::value<string>()->default_value("zeromq"),      "Transport ('zeromq'/'nanomsg').")
            ("config",            po::value<string>()->default_value("static"),      "Config source ('static'/<config library filename>).")
            ("control",           po::value<string>()->default_value("interactive"), "States control ('interactive'/'static'/<control library filename>).")
            ("network-interface", po::value<string>()->default_value("eth0"),        "Network interface to bind on (e.g. eth0, ib0, wlan0, en0, lo...).")
            ("config-key",        po::value<string>(),                               "Use provided value instead of device id for fetching the configuration from the config file")
            ("catch-signals",     po::value<int   >()->default_value(1),             "Enable signal handling (1/0)")
            ("log-to-file",       po::value<string>()->default_value(""),            "Log output to a file")
            ;

        fMQOptionsInCfg.add_options()
            ("id",                po::value<string>()->required(),                   "Device ID (required argument).")
            ("io-threads",        po::value<int   >()->default_value(1),             "Number of I/O threads.")
            ("transport",         po::value<string>()->default_value("zeromq"),      "Transport ('zeromq'/'nanomsg').")
            ("config",            po::value<string>()->default_value("static"),      "Config source ('static'/<config library filename>).")
            ("control",           po::value<string>()->default_value("interactive"), "States control ('interactive'/'static'/<control library filename>).")
            ("network-interface", po::value<string>()->default_value("eth0"),        "Network interface to bind on (e.g. eth0, ib0, wlan0, en0, lo...).")
            ("config-key",        po::value<string>(),                               "Use provided value instead of device id for fetching the configuration from the config file")
            ("catch-signals",     po::value<int   >()->default_value(1),             "Enable signal handling (1/0)")
            ("log-to-file",       po::value<string>()->default_value(""),            "Log output to a file")
            ;
    }
    else
    {
        fMQOptionsInCmd.add_options()
            ("id",                po::value<string>()->required(),                   "Device ID (required argument)")
            ("io-threads",        po::value<int   >()->default_value(1),             "Number of I/O threads")
            ("transport",         po::value<string>()->default_value("zeromq"),      "Transport ('zeromq'/'nanomsg').")
            ("config",            po::value<string>()->default_value("static"),      "Config source ('static'/<config library filename>).")
            ("control",           po::value<string>()->default_value("interactive"), "States control ('interactive'/'static'/<control library filename>).")
            ("network-interface", po::value<string>()->default_value("eth0"),        "Network interface to bind on (e.g. eth0, ib0, wlan0, en0, lo...).")
            ("config-key",        po::value<string>(),                               "Use provided value instead of device id for fetching the configuration from the config file")
            ("catch-signals",     po::value<int   >()->default_value(1),             "Enable signal handling (1/0)")
            ("log-to-file",       po::value<string>()->default_value(""),            "Log output to a file")
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

int FairMQProgOptions::UpdateChannelMap(const string& channelName, int index, const string& member, const string& val)
{
    if (member == "type")
    {
        fFairMQMap.at(channelName).at(index).UpdateType(val);
        return 0;
    }

    if (member == "method")
    {
        fFairMQMap.at(channelName).at(index).UpdateMethod(val);
        return 0;
    }

    if (member == "address")
    {
        fFairMQMap.at(channelName).at(index).UpdateAddress(val);
        return 0;
    }
    else
    {
        //if we get there it means something is wrong
        LOG(ERROR)  << "update of FairMQChannel map failed for the following key: "
                    << channelName << "." << index << "." << member;
        return 1;
    }
}

/*
// string API
int FairMQProgOptions::UpdateChannelMap(const string& channelName, int index, const string& member, const string& val)
{
    if (member == "type") 
    {
        fFairMQMap.at(channelName).at(index).UpdateType(val);
        return 0;
    }
    
    if (member == "method") 
    {
        fFairMQMap.at(channelName).at(index).UpdateMethod(val);
        return 0;
    }
    
    if (member == "address") 
    {
        fFairMQMap.at(channelName).at(index).UpdateAddress(val);
        return 0;
    }
    else
    {
        if (member == "sndBufSize" || member == "rcvBufSize" || member == "rateLogging") 
        {
            UpdateChannelMap(channelName,index,member,ConvertTo<int>(val));
        }

        //if we get there it means something is wrong
        LOG(ERROR)  << "update of FairMQChannel map failed for the following key: "
                    << channelName<<"."<<index<<"."<<member;
        return 1;
    }

}
*/
// ----------------------------------------------------------------------------------


int FairMQProgOptions::UpdateChannelMap(const string& channelName, int index, const string& member, int val)
{
    if (member == "sndBufSize")
    {
        fFairMQMap.at(channelName).at(index).UpdateSndBufSize(val);
        return 0;
    }

    if (member == "rcvBufSize")
    {
        fFairMQMap.at(channelName).at(index).UpdateRcvBufSize(val);
        return 0;
    }

    if (member == "rateLogging")
    {
        fFairMQMap.at(channelName).at(index).UpdateRateLogging(val);
        return 0;
    }
    else
    {
        //if we get there it means something is wrong
        LOG(ERROR)  << "update of FairMQChannel map failed for the following key: "
                    << channelName << "." << index << "." << member;
        return 1;
    }
}

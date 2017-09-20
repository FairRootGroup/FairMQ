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
#include "FairMQSuboptParser.h"
#include "FairMQLogger.h"
#include <iostream>

using namespace std;

FairMQProgOptions::FairMQProgOptions()
    : FairProgOptions(), FairMQEventManager()
    , fMQParserOptions("MQ-Device parser options")
    , fMQOptionsInCfg("MQ-Device options")
    , fMQOptionsInCmd("MQ-Device options")
    , fFairMQMap()
    , fHelpTitle("***** FAIRMQ Program Options ***** ")
    , fVersion("Beta version 0.1")
    , fChannelInfo()
    , fMQKeyMap()
    // , fSignalMap() //string API
{
}

FairMQProgOptions::~FairMQProgOptions()
{
}

void FairMQProgOptions::ParseAll(const int argc, char const* const* argv, bool allowUnregistered)
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

    if (fVarMap.count("print-options"))
    {
        PrintOptionsRaw();
        exit(EXIT_SUCCESS);
    }

    // if these options are provided, do no further checks and let the device handle them
    if (fVarMap.count("print-channels") || fVarMap.count("version"))
    {
        fair::mq::logger::DefaultConsoleSetFilter(fSeverityMap.at("NOLOG"));
        return;
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
        fair::mq::logger::ReinitLogger(false, logFile, fSeverityMap.at(verbosity));
        fair::mq::logger::DefaultConsoleSetFilter(fSeverityMap.at("NOLOG"));
    }
    else
    {
        if (!color)
        {
            fair::mq::logger::ReinitLogger(false);
        }

        fair::mq::logger::DefaultConsoleSetFilter(fSeverityMap.at(verbosity));
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
        LOG(WARN) << "FairMQProgOptions: no channels configuration provided via neither of:";
        for (const auto& p : MQParserKeys)
        {
            LOG(WARN) << " --" << p;
        }
        LOG(WARN) << "No channels will be created (You can create them manually).";
    }
    else
    {
        string id;

        if (fVarMap.count("config-key"))
        {
            id = fVarMap["config-key"].as<string>();
        }
        else
        {
            id = fVarMap["id"].as<string>();
        }

        // if cmdline mq-config called then use the default xml/json parser
        if (fVarMap.count("mq-config"))
        {
            LOG(DEBUG) << "mq-config: Using default XML/JSON parser";

            string file = fVarMap["mq-config"].as<string>();

            string ext = boost::filesystem::extension(file);

            transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".json")
            {
                UserParser<FairMQParser::JSON>(file, id);
            }
            else
            {
                if (ext == ".xml")
                {
                    UserParser<FairMQParser::XML>(file, id);
                }
                else
                {
                    LOG(ERROR)  << "mq-config command line called but file extension '"
                                << ext
                                << "' not recognized. Program will now exit";
                    exit(EXIT_FAILURE);
                }
            }
        }
        else if (fVarMap.count("config-json-string"))
        {
            LOG(DEBUG) << "config-json-string: Parsing JSON string";

            string value = FairMQ::ConvertVariableValue<FairMQ::ToString>().Run(fVarMap.at("config-json-string"));
            stringstream ss;
            ss << value;
            UserParser<FairMQParser::JSON>(ss, id);
        }
        else if (fVarMap.count("config-xml-string"))
        {
            LOG(DEBUG) << "config-json-string: Parsing XML string";

            string value = FairMQ::ConvertVariableValue<FairMQ::ToString>().Run(fVarMap.at("config-xml-string"));
            stringstream ss;
            ss << value;
            UserParser<FairMQParser::XML>(ss, id);
        }
        else if (fVarMap.count(FairMQParser::SUBOPT::OptionKeyChannelConfig))
        {
            LOG(DEBUG) << "channel-config: Parsing channel configuration";
            UserParser<FairMQParser::SUBOPT>(fVarMap, id);
        }
    }

    FairProgOptions::PrintOptions();
}

int FairMQProgOptions::Store(const FairMQMap& channels)
{
    fFairMQMap = channels;
    UpdateChannelInfo();
    UpdateMQValues();
    return 0;
}

// replace FairMQChannelMap, and update variable map accordingly
int FairMQProgOptions::UpdateChannelMap(const FairMQMap& channels)
{
    fFairMQMap = channels;
    UpdateChannelInfo();
    UpdateMQValues();
    return 0;
}

void FairMQProgOptions::UpdateChannelInfo()
{
    fChannelInfo.clear();
    for (const auto& c : fFairMQMap)
    {
        fChannelInfo.insert(std::make_pair(c.first, c.second.size()));
    }
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
            string typeKey = "chans." + p.first + "." + to_string(index) + ".type";
            string methodKey = "chans." + p.first + "." + to_string(index) + ".method";
            string addressKey = "chans." + p.first + "." + to_string(index) + ".address";
            string transportKey = "chans." + p.first + "." + to_string(index) + ".transport";
            string sndBufSizeKey = "chans." + p.first + "." + to_string(index) + ".sndBufSize";
            string rcvBufSizeKey = "chans." + p.first + "." + to_string(index) + ".rcvBufSize";
            string sndKernelSizeKey = "chans." + p.first + "." + to_string(index) + ".sndKernelSize";
            string rcvKernelSizeKey = "chans." + p.first + "." + to_string(index) + ".rcvKernelSize";
            string rateLoggingKey = "chans." + p.first + "." + to_string(index) + ".rateLogging";

            fMQKeyMap[typeKey] = make_tuple(p.first, index, "type");
            fMQKeyMap[methodKey] = make_tuple(p.first, index, "method");
            fMQKeyMap[addressKey] = make_tuple(p.first, index, "address");
            fMQKeyMap[transportKey] = make_tuple(p.first, index, "transport");
            fMQKeyMap[sndBufSizeKey] = make_tuple(p.first, index, "sndBufSize");
            fMQKeyMap[rcvBufSizeKey] = make_tuple(p.first, index, "rcvBufSize");
            fMQKeyMap[sndKernelSizeKey] = make_tuple(p.first, index, "sndKernelSize");
            fMQKeyMap[rcvKernelSizeKey] = make_tuple(p.first, index, "rcvkernelSize");
            fMQKeyMap[rateLoggingKey] = make_tuple(p.first, index, "rateLogging");

            UpdateVarMap<string>(typeKey, channel.GetType());
            UpdateVarMap<string>(methodKey, channel.GetMethod());
            UpdateVarMap<string>(addressKey, channel.GetAddress());
            UpdateVarMap<string>(transportKey, channel.GetTransport());

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
        std::cout << fHelpTitle << std::endl << fVisibleOptions;
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
            ("id",                     po::value<string>(),                                     "Device ID (required argument).")
            ("io-threads",             po::value<int   >()->default_value(1),                   "Number of I/O threads.")
            ("transport",              po::value<string>()->default_value("zeromq"),            "Transport ('zeromq'/'nanomsg').")
            ("config",                 po::value<string>()->default_value("static"),            "Config source ('static'/<config library filename>).")
            ("network-interface",      po::value<string>()->default_value("default"),           "Network interface to bind on (e.g. eth0, ib0..., default will try to detect the interface of the default route).")
            ("config-key",             po::value<string>(),                                     "Use provided value instead of device id for fetching the configuration from the config file.")
            ("catch-signals",          po::value<int   >()->default_value(1),                   "Enable signal handling (1/0).")
            ("initialization-timeout", po::value<int   >()->default_value(120),                 "Timeout for the initialization in seconds (when expecting dynamic initialization).")
            ("port-range-min",         po::value<int   >()->default_value(22000),               "Start of the port range for dynamic initialization.")
            ("port-range-max",         po::value<int   >()->default_value(32000),               "End of the port range for dynamic initialization.")
            ("log-to-file",            po::value<string>()->default_value(""),                  "Log output to a file.")
            ("print-channels",         po::value<bool  >()->implicit_value(true),               "Print registered channel endpoints in a machine-readable format (<channel name>:<min num subchannels>:<max num subchannels>)")
            ("shm-segment-size",       po::value<size_t>()->default_value(2000000000),          "shmem transport: size of the shared memory segment (in bytes).")
            ("shm-segment-name",       po::value<string>()->default_value("fairmq_shmem_main"), "shmem transport: name of the shared memory segment.")
            ;

        fMQOptionsInCfg.add_options()
            ("id",                     po::value<string>(),                                     "Device ID (required argument).")
            ("io-threads",             po::value<int   >()->default_value(1),                   "Number of I/O threads.")
            ("transport",              po::value<string>()->default_value("zeromq"),            "Transport ('zeromq'/'nanomsg').")
            ("config",                 po::value<string>()->default_value("static"),            "Config source ('static'/<config library filename>).")
            ("network-interface",      po::value<string>()->default_value("default"),           "Network interface to bind on (e.g. eth0, ib0..., default will try to detect the interface of the default route).")
            ("config-key",             po::value<string>(),                                     "Use provided value instead of device id for fetching the configuration from the config file.")
            ("catch-signals",          po::value<int   >()->default_value(1),                   "Enable signal handling (1/0).")
            ("initialization-timeout", po::value<int   >()->default_value(120),                 "Timeout for the initialization in seconds (when expecting dynamic initialization).")
            ("port-range-min",         po::value<int   >()->default_value(22000),               "Start of the port range for dynamic initialization.")
            ("port-range-max",         po::value<int   >()->default_value(32000),               "End of the port range for dynamic initialization.")
            ("log-to-file",            po::value<string>()->default_value(""),                  "Log output to a file.")
            ("print-channels",         po::value<bool  >()->implicit_value(true),               "Print registered channel endpoints in a machine-readable format (<channel name>:<min num subchannels>:<max num subchannels>)")
            ("shm-segment-size",       po::value<size_t>()->default_value(2000000000),          "shmem transport: size of the shared memory segment (in bytes).")
            ("shm-segment-name",       po::value<string>()->default_value("fairmq_shmem_main"), "shmem transport: name of the shared memory segment.")
            ;
    }
    else
    {
        fMQOptionsInCmd.add_options()
            ("id",                     po::value<string>(),                                     "Device ID (required argument).")
            ("io-threads",             po::value<int   >()->default_value(1),                   "Number of I/O threads.")
            ("transport",              po::value<string>()->default_value("zeromq"),            "Transport ('zeromq'/'nanomsg').")
            ("config",                 po::value<string>()->default_value("static"),            "Config source ('static'/<config library filename>).")
            ("network-interface",      po::value<string>()->default_value("default"),           "Network interface to bind on (e.g. eth0, ib0..., default will try to detect the interface of the default route).")
            ("config-key",             po::value<string>(),                                     "Use provided value instead of device id for fetching the configuration from the config file.")
            ("catch-signals",          po::value<int   >()->default_value(1),                   "Enable signal handling (1/0).")
            ("initialization-timeout", po::value<int   >()->default_value(120),                 "Timeout for the initialization in seconds (when expecting dynamic initialization).")
            ("port-range-min",         po::value<int   >()->default_value(22000),               "Start of the port range for dynamic initialization.")
            ("port-range-max",         po::value<int   >()->default_value(32000),               "End of the port range for dynamic initialization.")
            ("log-to-file",            po::value<string>()->default_value(""),                  "Log output to a file.")
            ("print-channels",         po::value<bool  >()->implicit_value(true),               "Print registered channel endpoints in a machine-readable format (<channel name>:<min num subchannels>:<max num subchannels>)")
            ("shm-segment-size",       po::value<size_t>()->default_value(2000000000),          "shmem transport: size of the shared memory segment (in bytes).")
            ("shm-segment-name",       po::value<string>()->default_value("fairmq_shmem_main"), "shmem transport: name of the shared memory segment.")
            ;
    }

    fMQParserOptions.add_options()
        ("config-xml-string",  po::value<vector<string>>()->multitoken(), "XML input as command line string.")
        // ("config-xml-file",    po::value<string>(),                       "XML input as file.")
        ("config-json-string", po::value<vector<string>>()->multitoken(), "JSON input as command line string.")
        // ("config-json-file",   po::value<string>(),                       "JSON input as file.")
        ("mq-config",          po::value<string>(),                       "JSON/XML input as file. The configuration object will check xml or json file extention and will call the json or xml parser accordingly")
        (FairMQParser::SUBOPT::OptionKeyChannelConfig, po::value<std::vector<std::string>>()->multitoken()->composing(), "Configuration of single or multiple channel(s) by comma separated key=value list")
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

    if (member == "transport")
    {
        fFairMQMap.at(channelName).at(index).UpdateTransport(val);
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

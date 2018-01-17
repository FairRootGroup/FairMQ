/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/*
 * File:   FairMQProgOptions.cxx
 * Author: winckler
 *
 * Created on March 11, 2015, 10:20 PM
 */

#include "FairMQProgOptions.h"
#include "FairMQParser.h"
#include "FairMQSuboptParser.h"
#include "FairMQLogger.h"

#include <algorithm>
#include <iostream>

using namespace std;

FairMQProgOptions::FairMQProgOptions()
    : FairProgOptions()
    , fMQParserOptions("FairMQ config parser options")
    , fMQCmdOptions("FairMQ device options")
    , fFairMQMap()
    , fChannelInfo()
    , fMQKeyMap()
{
}

FairMQProgOptions::~FairMQProgOptions()
{
}

int FairMQProgOptions::ParseAll(const vector<string>& cmdLineArgs, bool allowUnregistered)
{
    vector<const char*> argv(cmdLineArgs.size());

    transform(cmdLineArgs.begin(), cmdLineArgs.end(), argv.begin(), [](const string& str)
    {
        return str.c_str();
    });

    return ParseAll(argv.size(), const_cast<char**>(argv.data()), allowUnregistered);
}

int FairMQProgOptions::ParseAll(const int argc, char const* const* argv, bool allowUnregistered)
{
    InitOptionDescription();

    if (FairProgOptions::ParseCmdLine(argc, argv, fCmdLineOptions, fVarMap, allowUnregistered))
    {
        // ParseCmdLine returns 0 if no immediate switches found.
        return 1;
    }

    // if these options are provided, do no further checks and let the device handle them
    if (fVarMap.count("print-channels") || fVarMap.count("version"))
    {
        fair::Logger::SetConsoleSeverity("nolog");
        return 0;
    }

    string severity = GetValue<string>("severity");
    string logFile = GetValue<string>("log-to-file");
    bool color = GetValue<bool>("color");

    string verbosity = GetValue<string>("verbosity");
    fair::Logger::SetVerbosity(verbosity);

    if (logFile != "")
    {
        fair::Logger::InitFileSink(severity, logFile);
        fair::Logger::SetConsoleSeverity("nolog");
    }
    else
    {
        fair::Logger::SetConsoleColor(color);
        fair::Logger::SetConsoleSeverity(severity);
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
        LOG(warn) << "FairMQProgOptions: no channels configuration provided via neither of:";
        for (const auto& p : MQParserKeys)
        {
            LOG(warn) << " --" << p;
        }
        LOG(warn) << "No channels will be created (You can create them manually).";
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
            LOG(debug) << "mq-config: Using default XML/JSON parser";

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
                    LOG(error)  << "mq-config command line called but file extension '" << ext << "' not recognized. Program will now exit";
                    return 1;
                }
            }
        }
        else if (fVarMap.count("config-json-string"))
        {
            LOG(debug) << "config-json-string: Parsing JSON string";

            string value = FairMQ::ConvertVariableValue<FairMQ::ToString>().Run(fVarMap.at("config-json-string"));
            stringstream ss;
            ss << value;
            UserParser<FairMQParser::JSON>(ss, id);
        }
        else if (fVarMap.count("config-xml-string"))
        {
            LOG(debug) << "config-json-string: Parsing XML string";

            string value = FairMQ::ConvertVariableValue<FairMQ::ToString>().Run(fVarMap.at("config-xml-string"));
            stringstream ss;
            ss << value;
            UserParser<FairMQParser::XML>(ss, id);
        }
        else if (fVarMap.count(FairMQParser::SUBOPT::OptionKeyChannelConfig))
        {
            LOG(debug) << "channel-config: Parsing channel configuration";
            UserParser<FairMQParser::SUBOPT>(fVarMap, id);
        }
    }

    FairProgOptions::PrintOptions();

    return 0;
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
        fChannelInfo.insert(make_pair(c.first, c.second.size()));
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
            UpdateVarMap<int>(sndBufSizeKey, channel.GetSndBufSize());
            UpdateVarMap<int>(rcvBufSizeKey, channel.GetRcvBufSize());
            UpdateVarMap<int>(sndKernelSizeKey, channel.GetSndKernelSize());
            UpdateVarMap<int>(rcvKernelSizeKey, channel.GetRcvKernelSize());
            UpdateVarMap<int>(rateLoggingKey, channel.GetRateLogging());

            index++;
        }
        UpdateVarMap<int>("chans." + p.first + ".numSockets", index);
    }
}

int FairMQProgOptions::ImmediateOptions()
{
    if (fVarMap.count("help"))
    {
        cout << "===== FairMQ Program Options =====" << endl << fVisibleOptions;
        return 1;
    }

    if (fVarMap.count("print-options"))
    {
        PrintOptionsRaw();
        return 1;
    }

    return 0;
}

void FairMQProgOptions::InitOptionDescription()
{
    fMQCmdOptions.add_options()
        ("id",                     po::value<string>(),                            "Device ID (required argument).")
        ("io-threads",             po::value<int   >()->default_value(1),          "Number of I/O threads.")
        ("transport",              po::value<string>()->default_value("zeromq"),   "Transport ('zeromq'/'nanomsg').")
        ("config",                 po::value<string>()->default_value("static"),   "Config source ('static'/<config library filename>).")
        ("network-interface",      po::value<string>()->default_value("default"),  "Network interface to bind on (e.g. eth0, ib0..., default will try to detect the interface of the default route).")
        ("config-key",             po::value<string>(),                            "Use provided value instead of device id for fetching the configuration from the config file.")
        ("initialization-timeout", po::value<int   >()->default_value(120),        "Timeout for the initialization in seconds (when expecting dynamic initialization).")
        ("port-range-min",         po::value<int   >()->default_value(22000),      "Start of the port range for dynamic initialization.")
        ("port-range-max",         po::value<int   >()->default_value(32000),      "End of the port range for dynamic initialization.")
        ("log-to-file",            po::value<string>()->default_value(""),         "Log output to a file.")
        ("print-channels",         po::value<bool  >()->implicit_value(true),      "Print registered channel endpoints in a machine-readable format (<channel name>:<min num subchannels>:<max num subchannels>)")
        ("shm-segment-size",       po::value<size_t>()->default_value(2000000000), "Shared memory: size of the shared memory segment (in bytes).")
        ("rate",                   po::value<float >()->default_value(0.),         "Rate for conditional run loop (Hz).")
        ("session",                po::value<string>()->default_value("default"),  "Session name.")
        ;

    fMQParserOptions.add_options()
        ("config-xml-string",  po::value<vector<string>>()->multitoken(), "XML input as command line string.")
        ("config-json-string", po::value<vector<string>>()->multitoken(), "JSON input as command line string.")
        ("mq-config",          po::value<string>(),                       "JSON/XML input as file. The configuration object will check xml or json file extention and will call the json or xml parser accordingly")
        (FairMQParser::SUBOPT::OptionKeyChannelConfig, po::value<vector<string>>()->multitoken()->composing(), "Configuration of single or multiple channel(s) by comma separated key=value list")
        ;

    AddToCmdLineOptions(fGeneralDesc);
    AddToCmdLineOptions(fMQCmdOptions);
    AddToCmdLineOptions(fMQParserOptions);
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
        LOG(error)  << "update of FairMQChannel map failed for the following key: "
                    << channelName << "." << index << "." << member;
        return 1;
    }
}

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
        // if we get there it means something is wrong
        LOG(error)  << "update of FairMQChannel map failed for the following key: " << channelName << "." << index << "." << member;
        return 1;
    }
}

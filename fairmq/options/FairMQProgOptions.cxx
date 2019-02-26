/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
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

#include "FairMQLogger.h"
#include "FairMQProgOptions.h"
#include "FairProgOptionsHelper.h"

#include "FairMQParser.h"
#include "FairMQSuboptParser.h"

#include "tools/Unique.h"

#include <boost/algorithm/string.hpp> // join/split

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <exception>

using namespace std;
using namespace fair::mq;

namespace po = boost::program_options;

FairMQProgOptions::FairMQProgOptions()
    : fVarMap()
    , fFairMQChannelMap()
    , fAllOptions("FairMQ Command Line Options")
    , fGeneralOptions("General options")
    , fMQOptions("FairMQ device options")
    , fParserOptions("FairMQ channel config parser options")
    , fConfigMutex()
    , fChannelInfo()
    , fChannelKeyMap()
    , fUnregisteredOptions()
    , fEvents()
{
    fGeneralOptions.add_options()
        ("help,h",                                                      "Print help")
        ("version,v",                                                   "Print version")
        ("severity",      po::value<string>()->default_value("debug"),  "Log severity level: trace, debug, info, state, warn, error, fatal, nolog")
        ("verbosity",     po::value<string>()->default_value("medium"), "Log verbosity level: veryhigh, high, medium, low")
        ("color",         po::value<bool  >()->default_value(true),     "Log color (true/false)")
        ("log-to-file",   po::value<string>()->default_value(""),       "Log output to a file.")
        ("print-options", po::value<bool  >()->implicit_value(true),    "Print options in machine-readable format (<option>:<computed-value>:<type>:<description>)");

    fMQOptions.add_options()
        ("id",                     po::value<string  >(),                            "Device ID (required argument).")
        ("io-threads",             po::value<int     >()->default_value(1),          "Number of I/O threads.")
        ("transport",              po::value<string  >()->default_value("zeromq"),   "Transport ('zeromq'/'nanomsg'/'shmem').")
        ("network-interface",      po::value<string  >()->default_value("default"),  "Network interface to bind on (e.g. eth0, ib0..., default will try to detect the interface of the default route).")
        ("config-key",             po::value<string  >(),                            "Use provided value instead of device id for fetching the configuration from the config file.")
        ("initialization-timeout", po::value<int     >()->default_value(120),        "Timeout for the initialization in seconds (when expecting dynamic initialization).")
        ("max-run-time",           po::value<uint64_t>()->default_value(0),          "Maximum runtime for the Running state handler, after which state will change to Ready (in seconds, 0 for no limit).")
        ("print-channels",         po::value<bool    >()->implicit_value(true),      "Print registered channel endpoints in a machine-readable format (<channel name>:<min num subchannels>:<max num subchannels>)")
        ("shm-segment-size",       po::value<size_t  >()->default_value(2000000000), "Shared memory: size of the shared memory segment (in bytes).")
        ("shm-monitor",            po::value<bool    >()->default_value(true),       "Shared memory: run monitor daemon.")
        ("rate",                   po::value<float   >()->default_value(0.),         "Rate for conditional run loop (Hz).")
        ("session",                po::value<string  >()->default_value("default"),  "Session name.");

    fParserOptions.add_options()
        ("mq-config",      po::value<string>(),                                    "JSON input as file.")
        ("channel-config", po::value<vector<string>>()->multitoken()->composing(), "Configuration of single or multiple channel(s) by comma separated key=value list");

    fAllOptions.add(fGeneralOptions);
    fAllOptions.add(fMQOptions);
    fAllOptions.add(fParserOptions);

    ParseDefaults();
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
    ParseCmdLine(argc, argv, allowUnregistered);

    // if this option is provided, handle them and return stop value
    if (fVarMap.count("help"))
    {
        cout << fAllOptions << endl;
        return 1;
    }
    // if this option is provided, handle them and return stop value
    if (fVarMap.count("print-options"))
    {
        PrintOptionsRaw();
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

    string idForParser;

    // check if config-key for config parser is provided
    if (fVarMap.count("config-key"))
    {
        idForParser = fVarMap["config-key"].as<string>();
    }
    else if (fVarMap.count("id"))
    {
        idForParser = fVarMap["id"].as<string>();
    }

    // check if any config parser is selected
    try
    {
        if (fVarMap.count("mq-config"))
        {
            LOG(debug) << "mq-config: Using default JSON parser";
            UpdateChannelMap(parser::JSON().UserParser(fVarMap.at("mq-config").as<string>(), idForParser));
        }
        else if (fVarMap.count("channel-config"))
        {
            LOG(debug) << "channel-config: Parsing channel configuration";
            ParseChannelsFromCmdLine();
        }
        else
        {
            LOG(warn) << "FairMQProgOptions: no channels configuration provided via neither of:";
            for (const auto& p : fParserOptions.options())
            {
                LOG(warn) << "--" << p->canonical_display_name();
            }
        }
    }
    catch (exception& e)
    {
        LOG(error) << e.what();
        return 1;
    }

    PrintOptions();

    return 0;
}

void FairMQProgOptions::ParseChannelsFromCmdLine()
{
    string idForParser;

    // check if config-key for config parser is provided
    if (fVarMap.count("config-key"))
    {
        idForParser = fVarMap["config-key"].as<string>();
    }
    else if (fVarMap.count("id"))
    {
        idForParser = fVarMap["id"].as<string>();
    }

    UpdateChannelMap(parser::SUBOPT().UserParser(fVarMap.at("channel-config").as<vector<string>>(), idForParser));
}

void FairMQProgOptions::ParseCmdLine(const int argc, char const* const* argv, bool allowUnregistered)
{
    fVarMap.clear();

    // get options from cmd line and store in variable map
    // here we use command_line_parser instead of parse_command_line, to allow unregistered and positional options
    if (allowUnregistered)
    {
        po::command_line_parser parser{argc, argv};
        parser.options(fAllOptions).allow_unregistered();
        po::parsed_options parsed = parser.run();
        fUnregisteredOptions = po::collect_unrecognized(parsed.options, po::include_positional);

        po::store(parsed, fVarMap);
    }
    else
    {
        po::store(po::parse_command_line(argc, argv, fAllOptions), fVarMap);
    }

    po::notify(fVarMap);
}

void FairMQProgOptions::ParseDefaults()
{
    vector<string> emptyArgs = {"dummy", "--id", tools::Uuid()};

    vector<const char*> argv(emptyArgs.size());

    transform(emptyArgs.begin(), emptyArgs.end(), argv.begin(), [](const string& str)
    {
        return str.c_str();
    });

    po::store(po::parse_command_line(argv.size(), const_cast<char**>(argv.data()), fAllOptions), fVarMap);
}

unordered_map<string, vector<FairMQChannel>> FairMQProgOptions::GetFairMQMap() const
{
    return fFairMQChannelMap;
}

unordered_map<string, int> FairMQProgOptions::GetChannelInfo() const
{
    return fChannelInfo;
}

// replace FairMQChannelMap, and update variable map accordingly
int FairMQProgOptions::UpdateChannelMap(const unordered_map<string, vector<FairMQChannel>>& channels)
{
    fFairMQChannelMap = channels;
    UpdateChannelInfo();
    UpdateMQValues();
    return 0;
}

void FairMQProgOptions::UpdateChannelInfo()
{
    fChannelInfo.clear();
    for (const auto& c : fFairMQChannelMap)
    {
        fChannelInfo.insert(make_pair(c.first, c.second.size()));
    }
}

// read FairMQChannelMap and insert/update corresponding values in variable map
// create key for variable map as follow : channelName.index.memberName
void FairMQProgOptions::UpdateMQValues()
{
    for (const auto& p : fFairMQChannelMap) {
        int index = 0;

        for (const auto& channel : p.second) {
            string typeKey = "chans." + p.first + "." + to_string(index) + ".type";
            string methodKey = "chans." + p.first + "." + to_string(index) + ".method";
            string addressKey = "chans." + p.first + "." + to_string(index) + ".address";
            string transportKey = "chans." + p.first + "." + to_string(index) + ".transport";
            string sndBufSizeKey = "chans." + p.first + "." + to_string(index) + ".sndBufSize";
            string rcvBufSizeKey = "chans." + p.first + "." + to_string(index) + ".rcvBufSize";
            string sndKernelSizeKey = "chans." + p.first + "." + to_string(index) + ".sndKernelSize";
            string rcvKernelSizeKey = "chans." + p.first + "." + to_string(index) + ".rcvKernelSize";
            string lingerKey = "chans." + p.first + "." + to_string(index) + ".linger";
            string rateLoggingKey = "chans." + p.first + "." + to_string(index) + ".rateLogging";
            string portRangeMinKey = "chans." + p.first + "." + to_string(index) + ".portRangeMin";
            string portRangeMaxKey = "chans." + p.first + "." + to_string(index) + ".portRangeMax";
            string autoBindKey = "chans." + p.first + "." + to_string(index) + ".autoBind";

            fChannelKeyMap[typeKey] = ChannelKey{p.first, index, "type"};
            fChannelKeyMap[methodKey] = ChannelKey{p.first, index, "method"};
            fChannelKeyMap[addressKey] = ChannelKey{p.first, index, "address"};
            fChannelKeyMap[transportKey] = ChannelKey{p.first, index, "transport"};
            fChannelKeyMap[sndBufSizeKey] = ChannelKey{p.first, index, "sndBufSize"};
            fChannelKeyMap[rcvBufSizeKey] = ChannelKey{p.first, index, "rcvBufSize"};
            fChannelKeyMap[sndKernelSizeKey] = ChannelKey{p.first, index, "sndKernelSize"};
            fChannelKeyMap[rcvKernelSizeKey] = ChannelKey{p.first, index, "rcvkernelSize"};
            fChannelKeyMap[lingerKey] = ChannelKey{p.first, index, "linger"};
            fChannelKeyMap[rateLoggingKey] = ChannelKey{p.first, index, "rateLogging"};
            fChannelKeyMap[portRangeMinKey] = ChannelKey{p.first, index, "portRangeMin"};
            fChannelKeyMap[portRangeMaxKey] = ChannelKey{p.first, index, "portRangeMax"};
            fChannelKeyMap[autoBindKey] = ChannelKey{p.first, index, "autoBind"};

            UpdateVarMap<string>(typeKey, channel.GetType());
            UpdateVarMap<string>(methodKey, channel.GetMethod());
            UpdateVarMap<string>(addressKey, channel.GetAddress());
            UpdateVarMap<string>(transportKey, channel.GetTransportName());
            UpdateVarMap<int>(sndBufSizeKey, channel.GetSndBufSize());
            UpdateVarMap<int>(rcvBufSizeKey, channel.GetRcvBufSize());
            UpdateVarMap<int>(sndKernelSizeKey, channel.GetSndKernelSize());
            UpdateVarMap<int>(rcvKernelSizeKey, channel.GetRcvKernelSize());
            UpdateVarMap<int>(lingerKey, channel.GetLinger());
            UpdateVarMap<int>(rateLoggingKey, channel.GetRateLogging());
            UpdateVarMap<int>(portRangeMinKey, channel.GetPortRangeMin());
            UpdateVarMap<int>(portRangeMaxKey, channel.GetPortRangeMax());
            UpdateVarMap<bool>(autoBindKey, channel.GetAutoBind());

            index++;
        }

        UpdateVarMap<int>("chans." + p.first + ".numSockets", index);
    }
}

int FairMQProgOptions::UpdateChannelValue(const string& channelName, int index, const string& member, const string& val)
{
    if (member == "type") {
        fFairMQChannelMap.at(channelName).at(index).UpdateType(val);
    } else if (member == "method") {
        fFairMQChannelMap.at(channelName).at(index).UpdateMethod(val);
    } else if (member == "address") {
        fFairMQChannelMap.at(channelName).at(index).UpdateAddress(val);
    } else if (member == "transport") {
        fFairMQChannelMap.at(channelName).at(index).UpdateTransport(val);
    } else {
        LOG(error)  << "update of FairMQChannel map failed for the following key: " << channelName << "." << index << "." << member;
        return 1;
    }

    return 0;
}

int FairMQProgOptions::UpdateChannelValue(const string& channelName, int index, const string& member, int val)
{
    if (member == "sndBufSize") {
        fFairMQChannelMap.at(channelName).at(index).UpdateSndBufSize(val);
    } else if (member == "rcvBufSize") {
        fFairMQChannelMap.at(channelName).at(index).UpdateRcvBufSize(val);
    } else if (member == "sndKernelSize") {
        fFairMQChannelMap.at(channelName).at(index).UpdateSndKernelSize(val);
    } else if (member == "rcvKernelSize") {
        fFairMQChannelMap.at(channelName).at(index).UpdateRcvKernelSize(val);
    } else if (member == "linger") {
        fFairMQChannelMap.at(channelName).at(index).UpdateLinger(val);
    } else if (member == "rateLogging") {
        fFairMQChannelMap.at(channelName).at(index).UpdateRateLogging(val);
    } else if (member == "portRangeMin") {
        fFairMQChannelMap.at(channelName).at(index).UpdatePortRangeMin(val);
    } else if (member == "portRangeMax") {
        fFairMQChannelMap.at(channelName).at(index).UpdatePortRangeMax(val);
    } else {
        LOG(error)  << "update of FairMQChannel map failed for the following key: " << channelName << "." << index << "." << member;
        return 1;
    }

    return 0;
}

int FairMQProgOptions::UpdateChannelValue(const string& channelName, int index, const string& member, bool val)
{
    if (member == "autoBind") {
        fFairMQChannelMap.at(channelName).at(index).UpdateAutoBind(val);
        return 0;
    } else {
        LOG(error)  << "update of FairMQChannel map failed for the following key: " << channelName << "." << index << "." << member;
        return 1;
    }
}

vector<string> FairMQProgOptions::GetPropertyKeys() const
{
    lock_guard<mutex> lock{fConfigMutex};

    vector<string> result;

    for (const auto& it : fVarMap)
    {
        result.push_back(it.first.c_str());
    }

    return result;
}

/// Add option descriptions
int FairMQProgOptions::AddToCmdLineOptions(const po::options_description optDesc, bool /* visible */)
{
    fAllOptions.add(optDesc);
    return 0;
}

po::options_description& FairMQProgOptions::GetCmdLineOptions()
{
    return fAllOptions;
}

int FairMQProgOptions::PrintOptions()
{
    // -> loop over variable map and print its content
    // -> In this example the following types are supported:
    // string, int, float, double, short, boost::filesystem::path
    // vector<string>, vector<int>, vector<float>, vector<double>, vector<short>

    map<string, VarValInfo> mapinfo;

    // get string length for formatting and convert varmap values into string
    int maxLenKey = 0;
    int maxLenValue = 0;
    int maxLenType = 0;
    int maxLenDefault = 0;

    for (const auto& m : fVarMap)
    {
        maxLenKey = max(maxLenKey, static_cast<int>(m.first.length()));

        VarValInfo valinfo = ConvertVariableValue<options::ToVarValInfo>()((m.second));
        mapinfo[m.first] = valinfo;

        maxLenValue = max(maxLenValue, static_cast<int>(valinfo.value.length()));
        maxLenType = max(maxLenType, static_cast<int>(valinfo.type.length()));
        maxLenDefault = max(maxLenDefault, static_cast<int>(valinfo.defaulted.length()));
    }

    // TODO : limit the value len field in a better way
    if (maxLenValue > 100)
    {
        maxLenValue = 100;
    }

    for (const auto& o : fUnregisteredOptions)
    {
        LOG(debug) << "detected unregistered option: " << o;
    }

    stringstream ss;
    ss << "Configuration: \n";

    for (const auto& p : mapinfo)
    {
        ss << setfill(' ') << left
           << setw(maxLenKey) << p.first << " = "
           << setw(maxLenValue) << p.second.value << " "
           << setw(maxLenType) << p.second.type
           << setw(maxLenDefault) << p.second.defaulted
           << "\n";
    }

    LOG(debug) << ss.str();

    return 0;
}

int FairMQProgOptions::PrintOptionsRaw()
{
    const vector<boost::shared_ptr<po::option_description>>& options = fAllOptions.options();

    for (const auto& o : options)
    {
        VarValInfo value;
        if (fVarMap.count(o->canonical_display_name()))
        {
            value = ConvertVariableValue<options::ToVarValInfo>()((fVarMap[o->canonical_display_name()]));
        }

        string description = o->description();

        replace(description.begin(), description.end(), '\n', ' ');

        cout << o->long_name() << ":" << value.value << ":" << (value.type == "" ? "<unknown>" : value.type) << ":" << description << endl;
    }

    return 0;
}

string FairMQProgOptions::GetStringValue(const string& key)
{
    unique_lock<mutex> lock(fConfigMutex);

    string valueStr;
    try
    {
        if (fVarMap.count(key))
        {
            valueStr = ConvertVariableValue<options::ToString>()(fVarMap.at(key));
        }
    }
    catch (exception& e)
    {
        LOG(error) << "Exception thrown for the key '" << key << "'";
        LOG(error) << e.what();
    }

    return valueStr;
}

int FairMQProgOptions::Count(const string& key) const
{
    unique_lock<mutex> lock(fConfigMutex);

    return fVarMap.count(key);
}

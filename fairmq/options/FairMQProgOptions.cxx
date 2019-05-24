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

#include "FairMQParser.h"
#include "FairMQSuboptParser.h"

#include "tools/Unique.h"

#include <boost/filesystem.hpp>
#include <boost/any.hpp>
#include <boost/algorithm/string.hpp> // join/split
#include <boost/regex.hpp>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <exception>

using namespace std;
using namespace fair::mq;
using boost::any_cast;

namespace po = boost::program_options;

struct ValInfo
{
    string value;
    string type;
    string origin;
};

template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    for (unsigned int i = 0; i < v.size(); ++i) {
        os << v[i];
        if (i != v.size() - 1) {
            os << ", ";
        }
    }
    return os;
}

template<typename T>
pair<string, string> getString(const boost::any& v, const string& label)
{
    return { to_string(any_cast<T>(v)), label };
}


template<typename T>
pair<string, string> getStringPair(const boost::any& v, const string& label)
{
    stringstream ss;
    ss << any_cast<T>(v);
    return { ss.str(), label };
}

unordered_map<type_index, function<pair<string, string>(const Property&)>> FairMQProgOptions::fTypeInfos = {
    { type_index(typeid(char)),                            [](const Property& p) { return pair<string, string>{ string(1, any_cast<char>(p)), "char" }; } },
    { type_index(typeid(unsigned char)),                   [](const Property& p) { return pair<string, string>{ string(1, any_cast<unsigned char>(p)), "unsigned char" }; } },
    { type_index(typeid(string)),                          [](const Property& p) { return pair<string, string>{ any_cast<string>(p), "string" }; } },
    { type_index(typeid(int)),                             [](const Property& p) { return getString<int>(p, "int"); } },
    { type_index(typeid(size_t)),                          [](const Property& p) { return getString<size_t>(p, "size_t"); } },
    { type_index(typeid(uint32_t)),                        [](const Property& p) { return getString<uint32_t>(p, "uint32_t"); } },
    { type_index(typeid(uint64_t)),                        [](const Property& p) { return getString<uint64_t>(p, "uint64_t"); } },
    { type_index(typeid(long)),                            [](const Property& p) { return getString<long>(p, "long"); } },
    { type_index(typeid(long long)),                       [](const Property& p) { return getString<long long>(p, "long long"); } },
    { type_index(typeid(unsigned)),                        [](const Property& p) { return getString<unsigned>(p, "unsigned"); } },
    { type_index(typeid(unsigned long)),                   [](const Property& p) { return getString<unsigned long>(p, "unsigned long"); } },
    { type_index(typeid(unsigned long long)),              [](const Property& p) { return getString<unsigned long long>(p, "unsigned long long"); } },
    { type_index(typeid(float)),                           [](const Property& p) { return getString<float>(p, "float"); } },
    { type_index(typeid(double)),                          [](const Property& p) { return getString<double>(p, "double"); } },
    { type_index(typeid(long double)),                     [](const Property& p) { return getString<long double>(p, "long double"); } },
    { type_index(typeid(bool)),                            [](const Property& p) { stringstream ss; ss << boolalpha << any_cast<bool>(p); return pair<string, string>{ ss.str(), "bool" }; } },
    { type_index(typeid(vector<bool>)),                    [](const Property& p) { stringstream ss; ss << boolalpha << any_cast<vector<bool>>(p); return pair<string, string>{ ss.str(), "vector<bool>>" }; } },
    { type_index(typeid(boost::filesystem::path)),         [](const Property& p) { return getStringPair<boost::filesystem::path>(p, "boost::filesystem::path"); } },
    { type_index(typeid(vector<char>)),                    [](const Property& p) { return getStringPair<vector<char>>(p, "vector<char>"); } },
    { type_index(typeid(vector<unsigned char>)),           [](const Property& p) { return getStringPair<vector<unsigned char>>(p, "vector<unsigned char>"); } },
    { type_index(typeid(vector<string>)),                  [](const Property& p) { return getStringPair<vector<string>>(p, "vector<string>"); } },
    { type_index(typeid(vector<int>)),                     [](const Property& p) { return getStringPair<vector<int>>(p, "vector<int>"); } },
    { type_index(typeid(vector<size_t>)),                  [](const Property& p) { return getStringPair<vector<size_t>>(p, "vector<size_t>"); } },
    { type_index(typeid(vector<uint32_t>)),                [](const Property& p) { return getStringPair<vector<uint32_t>>(p, "vector<uint32_t>"); } },
    { type_index(typeid(vector<uint64_t>)),                [](const Property& p) { return getStringPair<vector<uint64_t>>(p, "vector<uint64_t>"); } },
    { type_index(typeid(vector<long>)),                    [](const Property& p) { return getStringPair<vector<long>>(p, "vector<long>"); } },
    { type_index(typeid(vector<long long>)),               [](const Property& p) { return getStringPair<vector<long long>>(p, "vector<long long>"); } },
    { type_index(typeid(vector<unsigned>)),                [](const Property& p) { return getStringPair<vector<unsigned>>(p, "vector<unsigned>"); } },
    { type_index(typeid(vector<unsigned long>)),           [](const Property& p) { return getStringPair<vector<unsigned long>>(p, "vector<unsigned long>"); } },
    { type_index(typeid(vector<unsigned long long>)),      [](const Property& p) { return getStringPair<vector<unsigned long long>>(p, "vector<unsigned long long>"); } },
    { type_index(typeid(vector<float>)),                   [](const Property& p) { return getStringPair<vector<float>>(p, "vector<float>"); } },
    { type_index(typeid(vector<double>)),                  [](const Property& p) { return getStringPair<vector<double>>(p, "vector<double>"); } },
    { type_index(typeid(vector<long double>)),             [](const Property& p) { return getStringPair<vector<long double>>(p, "vector<long double>"); } },
    { type_index(typeid(vector<boost::filesystem::path>)), [](const Property& p) { return getStringPair<vector<boost::filesystem::path>>(p, "vector<boost::filesystem::path>"); } },
};

unordered_map<type_index, void(*)(const EventManager&, const string&, const Property&)> FairMQProgOptions::fEventEmitters = {
    { type_index(typeid(char)),                            [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, char>(k, any_cast<char>(p)); } },
    { type_index(typeid(unsigned char)),                   [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, unsigned char>(k, any_cast<unsigned char>(p)); } },
    { type_index(typeid(string)),                          [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, string>(k, any_cast<string>(p)); } },
    { type_index(typeid(int)),                             [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, int>(k, any_cast<int>(p)); } },
    { type_index(typeid(size_t)),                          [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, size_t>(k, any_cast<size_t>(p)); } },
    { type_index(typeid(uint32_t)),                        [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, uint32_t>(k, any_cast<uint32_t>(p)); } },
    { type_index(typeid(uint64_t)),                        [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, uint64_t>(k, any_cast<uint64_t>(p)); } },
    { type_index(typeid(long)),                            [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, long>(k, any_cast<long>(p)); } },
    { type_index(typeid(long long)),                       [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, long long>(k, any_cast<long long>(p)); } },
    { type_index(typeid(unsigned)),                        [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, unsigned>(k, any_cast<unsigned>(p)); } },
    { type_index(typeid(unsigned long)),                   [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, unsigned long>(k, any_cast<unsigned long>(p)); } },
    { type_index(typeid(unsigned long long)),              [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, unsigned long long>(k, any_cast<unsigned long long>(p)); } },
    { type_index(typeid(float)),                           [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, float>(k, any_cast<float>(p)); } },
    { type_index(typeid(double)),                          [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, double>(k, any_cast<double>(p)); } },
    { type_index(typeid(long double)),                     [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, long double>(k, any_cast<long double>(p)); } },
    { type_index(typeid(bool)),                            [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, bool>(k, any_cast<bool>(p)); } },
    { type_index(typeid(vector<bool>)),                    [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<bool>>(k, any_cast<vector<bool>>(p)); } },
    { type_index(typeid(boost::filesystem::path)),         [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, boost::filesystem::path>(k, any_cast<boost::filesystem::path>(p)); } },
    { type_index(typeid(vector<char>)),                    [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<char>>(k, any_cast<vector<char>>(p)); } },
    { type_index(typeid(vector<unsigned char>)),           [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<unsigned char>>(k, any_cast<vector<unsigned char>>(p)); } },
    { type_index(typeid(vector<string>)),                  [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<string>>(k, any_cast<vector<string>>(p)); } },
    { type_index(typeid(vector<int>)),                     [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<int>>(k, any_cast<vector<int>>(p)); } },
    { type_index(typeid(vector<size_t>)),                  [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<size_t>>(k, any_cast<vector<size_t>>(p)); } },
    { type_index(typeid(vector<uint32_t>)),                [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<uint32_t>>(k, any_cast<vector<uint32_t>>(p)); } },
    { type_index(typeid(vector<uint64_t>)),                [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<uint64_t>>(k, any_cast<vector<uint64_t>>(p)); } },
    { type_index(typeid(vector<long>)),                    [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<long>>(k, any_cast<vector<long>>(p)); } },
    { type_index(typeid(vector<long long>)),               [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<long long>>(k, any_cast<vector<long long>>(p)); } },
    { type_index(typeid(vector<unsigned>)),                [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<unsigned>>(k, any_cast<vector<unsigned>>(p)); } },
    { type_index(typeid(vector<unsigned long>)),           [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<unsigned long>>(k, any_cast<vector<unsigned long>>(p)); } },
    { type_index(typeid(vector<unsigned long long>)),      [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<unsigned long long>>(k, any_cast<vector<unsigned long long>>(p)); } },
    { type_index(typeid(vector<float>)),                   [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<float>>(k, any_cast<vector<float>>(p)); } },
    { type_index(typeid(vector<double>)),                  [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<double>>(k, any_cast<vector<double>>(p)); } },
    { type_index(typeid(vector<long double>)),             [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<long double>>(k, any_cast<vector<long double>>(p)); } },
    { type_index(typeid(vector<boost::filesystem::path>)), [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<boost::filesystem::path>>(k, any_cast<vector<boost::filesystem::path>>(p)); } },
};

namespace fair
{
namespace mq
{

string ConvertPropertyToString(const Property& p)
{
    pair<string, string> info = FairMQProgOptions::fTypeInfos.at(p.type())(p);
    return info.first;
}

ValInfo ConvertVarValToValInfo(const po::variable_value& v)
{
    string origin;

    if (v.defaulted()) {
        origin = "[default]";
    } else if (v.empty()) {
        origin = "[empty]";
    } else {
        origin = "[provided]";
    }

    try {
        pair<string, string> info = FairMQProgOptions::fTypeInfos.at(v.value().type())(v.value());
         return {info.first, info.second, origin};
    } catch (out_of_range& oor) {
        return {string("[unidentified_type]"), string("[unidentified_type]"), origin};
    }
};

string ConvertVarValToString(const po::variable_value& v)
{
    return ConvertVarValToValInfo(v).value;
}

} // namespace mq
} // namespace fair

FairMQProgOptions::FairMQProgOptions()
    : fVarMap()
    , fAllOptions("FairMQ Command Line Options")
    , fGeneralOptions("General options")
    , fMQOptions("FairMQ device options")
    , fParserOptions("FairMQ channel config parser options")
    , fMtx()
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
        ("ofi-size-hint",          po::value<size_t  >()->default_value(0),          "EXPERIMENTAL: OFI size hint for the allocator.")
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

unordered_map<string, int> FairMQProgOptions::GetChannelInfo() const
{
    lock_guard<mutex> lock(fMtx);
    return GetChannelInfoImpl();
}

unordered_map<string, int> FairMQProgOptions::GetChannelInfoImpl() const
{
    unordered_map<string, int> info;

    boost::regex re("chans\\..*\\.type");
    for (const auto& m : fVarMap) {
        if (boost::regex_match(m.first, re)) {
            string chan = m.first.substr(6);
            string::size_type n = chan.find(".");
            string chanName = chan.substr(0, n);

            if (info.find(chanName) == info.end()) {
                info.emplace(chanName, 1);
            } else {
                info[chanName] = info[chanName] + 1;
            }
        }
    }

    return info;
}

Properties FairMQProgOptions::GetProperties(const string& q) const
{
    boost::regex re(q);
    Properties result;

    lock_guard<mutex> lock(fMtx);

    for (const auto& m : fVarMap) {
        if (boost::regex_match(m.first, re)) {
            result.emplace(m.first, m.second.value());
        }
    }

    if (result.size() == 0) {
        LOG(warn) << "could not find anything with \"" << q << "\"";
    }

    return result;
}

map<string, string> FairMQProgOptions::GetPropertiesAsString(const string& q) const
{
    boost::regex re(q);
    map<string, string> result;

    lock_guard<mutex> lock(fMtx);

    for (const auto& m : fVarMap) {
        if (boost::regex_match(m.first, re)) {
            result.emplace(m.first, ConvertPropertyToString(m.second.value()));
        }
    }

    if (result.size() == 0) {
        LOG(warn) << "could not find anything with \"" << q << "\"";
    }

    return result;
}

Properties FairMQProgOptions::GetPropertiesStartingWith(const string& q) const
{
    Properties result;

    lock_guard<mutex> lock(fMtx);

    for (const auto& m : fVarMap) {
        if (m.first.compare(0, q.length(), q) == 0) {
            result.emplace(m.first, m.second.value());
        }
    }

    return result;
}

void FairMQProgOptions::SetProperties(const Properties& input)
{
    unique_lock<mutex> lock(fMtx);

    map<string, boost::program_options::variable_value>& vm = fVarMap;
    for (const auto& m : input) {
        vm[m.first].value() = m.second;
    }

    lock.unlock();

    for (const auto& m : input) {
        fEventEmitters.at(m.second.type())(fEvents, m.first, m.second);
        fEvents.Emit<PropertyChangeAsString, string>(m.first, ConvertPropertyToString(m.second));
    }
}

void FairMQProgOptions::AddChannel(const std::string& name, const FairMQChannel& channel)
{
    lock_guard<mutex> lock(fMtx);
    unordered_map<string, int> existingChannels = GetChannelInfoImpl();
    int index = 0;
    if (existingChannels.count(name) > 0) {
        index = existingChannels.at(name);
    }

    string prefix = fair::mq::tools::ToString("chans.", name, ".", index, ".");

    SetVarMapValue<string>(string(prefix + "type"), channel.GetType());
    SetVarMapValue<string>(string(prefix + "method"), channel.GetMethod());
    SetVarMapValue<string>(string(prefix + "address"), channel.GetAddress());
    SetVarMapValue<string>(string(prefix + "transport"), channel.GetTransportName());
    SetVarMapValue<int>(string(prefix + "sndBufSize"), channel.GetSndBufSize());
    SetVarMapValue<int>(string(prefix + "rcvBufSize"), channel.GetRcvBufSize());
    SetVarMapValue<int>(string(prefix + "sndKernelSize"), channel.GetSndKernelSize());
    SetVarMapValue<int>(string(prefix + "rcvKernelSize"), channel.GetRcvKernelSize());
    SetVarMapValue<int>(string(prefix + "linger"), channel.GetLinger());
    SetVarMapValue<int>(string(prefix + "rateLogging"), channel.GetRateLogging());
    SetVarMapValue<int>(string(prefix + "portRangeMin"), channel.GetPortRangeMin());
    SetVarMapValue<int>(string(prefix + "portRangeMax"), channel.GetPortRangeMax());
    SetVarMapValue<bool>(string(prefix + "autoBind"), channel.GetAutoBind());
}

void FairMQProgOptions::DeleteProperty(const string& key)
{
    lock_guard<mutex> lock(fMtx);

    map<string, boost::program_options::variable_value>& vm = fVarMap;
    vm.erase(key);
}

int FairMQProgOptions::ParseAll(const vector<string>& cmdArgs, bool allowUnregistered)
{
    vector<const char*> argv(cmdArgs.size());
    transform(cmdArgs.begin(), cmdArgs.end(), argv.begin(), [](const string& str) {
        return str.c_str();
    });
    return ParseAll(argv.size(), const_cast<char**>(argv.data()), allowUnregistered);
}

int FairMQProgOptions::ParseAll(const int argc, char const* const* argv, bool allowUnregistered)
{
    ParseCmdLine(argc, argv, allowUnregistered);

    // if this option is provided, handle them and return stop value
    if (fVarMap.count("help")) {
        cout << fAllOptions << endl;
        return 1;
    }
    // if this option is provided, handle them and return stop value
    if (fVarMap.count("print-options")) {
        PrintOptionsRaw();
        return 1;
    }
    // if these options are provided, do no further checks and let the device handle them
    if (fVarMap.count("print-channels") || fVarMap.count("version")) {
        fair::Logger::SetConsoleSeverity("nolog");
        return 0;
    }

    string severity = GetValue<string>("severity");
    string logFile = GetValue<string>("log-to-file");
    bool color = GetValue<bool>("color");

    string verbosity = GetValue<string>("verbosity");
    fair::Logger::SetVerbosity(verbosity);

    if (logFile != "") {
        fair::Logger::InitFileSink(severity, logFile);
        fair::Logger::SetConsoleSeverity("nolog");
    } else {
        fair::Logger::SetConsoleColor(color);
        fair::Logger::SetConsoleSeverity(severity);
    }

    string idForParser;

    // check if config-key for config parser is provided
    if (fVarMap.count("config-key")) {
        idForParser = fVarMap["config-key"].as<string>();
    } else if (fVarMap.count("id")) {
        idForParser = fVarMap["id"].as<string>();
    }

    // check if any config parser is selected
    try {
        if (fVarMap.count("mq-config")) {
            LOG(debug) << "mq-config: Using default JSON parser";
            SetProperties(parser::JSON().UserParser(fVarMap.at("mq-config").as<string>(), idForParser));
        } else if (fVarMap.count("channel-config")) {
            LOG(debug) << "channel-config: Parsing channel configuration";
            ParseChannelsFromCmdLine();
        } else {
            LOG(warn) << "FairMQProgOptions: no channels configuration provided via neither of:";
            for (const auto& p : fParserOptions.options()) {
                LOG(warn) << "--" << p->canonical_display_name();
            }
        }
    } catch (exception& e) {
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
    if (fVarMap.count("config-key")) {
        idForParser = fVarMap["config-key"].as<string>();
    } else if (fVarMap.count("id")) {
        idForParser = fVarMap["id"].as<string>();
    }

    SetProperties(parser::SUBOPT().UserParser(fVarMap.at("channel-config").as<vector<string>>(), idForParser));
}

void FairMQProgOptions::ParseCmdLine(const int argc, char const* const* argv, bool allowUnregistered)
{
    // clear the container because it was filled with default values and subsequent calls to store() do not overwrite the existing values
    fVarMap.clear();

    if (allowUnregistered) {
        po::command_line_parser parser{argc, argv};
        parser.options(fAllOptions).allow_unregistered();
        po::parsed_options parsed = parser.run();
        fUnregisteredOptions = po::collect_unrecognized(parsed.options, po::include_positional);

        po::store(parsed, fVarMap);
    } else {
        po::store(po::parse_command_line(argc, argv, fAllOptions), fVarMap);
    }

    po::notify(fVarMap);
}

void FairMQProgOptions::ParseDefaults()
{
    vector<string> emptyArgs = {"dummy", "--id", tools::Uuid()};

    vector<const char*> argv(emptyArgs.size());

    transform(emptyArgs.begin(), emptyArgs.end(), argv.begin(), [](const string& str) {
        return str.c_str();
    });

    po::store(po::parse_command_line(argv.size(), const_cast<char**>(argv.data()), fAllOptions), fVarMap);
}

vector<string> FairMQProgOptions::GetPropertyKeys() const
{
    lock_guard<mutex> lock(fMtx);

    vector<string> result;

    for (const auto& it : fVarMap) {
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
    map<string, ValInfo> mapinfo;

    // get string length for formatting and convert varmap values into string
    int maxLenKey = 0;
    int maxLenValue = 0;
    int maxLenType = 0;
    int maxLenDefault = 0;

    for (const auto& m : fVarMap) {
        maxLenKey = max(maxLenKey, static_cast<int>(m.first.length()));

        ValInfo valinfo = ConvertVarValToValInfo(m.second);
        mapinfo[m.first] = valinfo;

        maxLenValue = max(maxLenValue, static_cast<int>(valinfo.value.length()));
        maxLenType = max(maxLenType, static_cast<int>(valinfo.type.length()));
        maxLenDefault = max(maxLenDefault, static_cast<int>(valinfo.origin.length()));
    }

    if (maxLenValue > 100) {
        maxLenValue = 100;
    }

    for (const auto& o : fUnregisteredOptions) {
        LOG(debug) << "detected unregistered option: " << o;
    }

    stringstream ss;
    ss << "Configuration: \n";

    for (const auto& p : mapinfo) {
        string type("<" + p.second.type + ">");
        ss << setfill(' ') << left
           << setw(maxLenKey) << p.first << " = "
           << setw(maxLenValue) << p.second.value << " "
           << setw(maxLenType + 2) << type << " "
           << setw(maxLenDefault) << p.second.origin
           << "\n";
    }

    LOG(debug) << ss.str();

    return 0;
}

int FairMQProgOptions::PrintOptionsRaw()
{
    const vector<boost::shared_ptr<po::option_description>>& options = fAllOptions.options();

    for (const auto& o : options) {
        ValInfo value;
        if (fVarMap.count(o->canonical_display_name())) {
            value = ConvertVarValToValInfo(fVarMap[o->canonical_display_name()]);
        }

        string description = o->description();

        replace(description.begin(), description.end(), '\n', ' ');

        cout << o->long_name() << ":" << value.value << ":" << (value.type == "" ? "<unknown>" : value.type) << ":" << description << endl;
    }

    return 0;
}

string FairMQProgOptions::GetPropertyAsString(const string& key) const
{
    lock_guard<mutex> lock(fMtx);

    if (fVarMap.count(key)) {
        return ConvertVarValToString(fVarMap.at(key));
    }

    throw PropertyNotFoundException(fair::mq::tools::ToString("Config has no key: ", key));
}

int FairMQProgOptions::Count(const string& key) const
{
    lock_guard<mutex> lock(fMtx);

    return fVarMap.count(key);
}

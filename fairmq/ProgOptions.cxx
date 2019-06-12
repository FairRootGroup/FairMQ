/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/*
 * File:   ProgOptions.cxx
 * Author: winckler
 *
 * Created on March 11, 2015, 10:20 PM
 */

#include "FairMQLogger.h"
#include <fairmq/ProgOptions.h>

#include "tools/Unique.h"

#include <boost/any.hpp>
#include <boost/regex.hpp>

#include <algorithm>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility> // pair

using namespace std;
using namespace fair::mq;

namespace po = boost::program_options;

struct ValInfo
{
    string value;
    string type;
    string origin;
};

namespace fair
{
namespace mq
{

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
        pair<string, string> info = PropertyHelper::GetPropertyInfo(v.value());
        return {info.first, info.second, origin};
    } catch (out_of_range& oor) {
        return {string("[unidentified_type]"), string("[unidentified_type]"), origin};
    }
};

string ConvertVarValToString(const po::variable_value& v)
{
    return ConvertVarValToValInfo(v).value;
}

ProgOptions::ProgOptions()
    : fVarMap()
    , fAllOptions("FairMQ Command Line Options")
    , fUnregisteredOptions()
    , fEvents()
    , fMtx()
{
    fAllOptions.add_options()
        ("help,h",                                                      "Print help")
        ("version,v",                                                   "Print version")
        ("severity",      po::value<string>()->default_value("debug"),  "Log severity level (console): trace, debug, info, state, warn, error, fatal, nolog")
        ("file-severity", po::value<string>()->default_value("debug"),  "Log severity level (file): trace, debug, info, state, warn, error, fatal, nolog")
        ("verbosity",     po::value<string>()->default_value("medium"), "Log verbosity level: veryhigh, high, medium, low")
        ("color",         po::value<bool  >()->default_value(true),     "Log color (true/false)")
        ("log-to-file",   po::value<string>()->default_value(""),       "Log output to a file.")
        ("print-options", po::value<bool  >()->implicit_value(true),    "Print options in machine-readable format (<option>:<computed-value>:<type>:<description>)");

    ParseDefaults();
}

void ProgOptions::ParseDefaults()
{
    vector<string> emptyArgs = {"dummy"};

    vector<const char*> argv(emptyArgs.size());

    transform(emptyArgs.begin(), emptyArgs.end(), argv.begin(), [](const string& str) {
        return str.c_str();
    });

    po::store(po::parse_command_line(argv.size(), const_cast<char**>(argv.data()), fAllOptions), fVarMap);
}

void ProgOptions::ParseAll(const vector<string>& cmdArgs, bool allowUnregistered)
{
    vector<const char*> argv(cmdArgs.size());
    transform(cmdArgs.begin(), cmdArgs.end(), argv.begin(), [](const string& str) {
        return str.c_str();
    });
    ParseAll(argv.size(), const_cast<char**>(argv.data()), allowUnregistered);
}

void ProgOptions::ParseAll(const int argc, char const* const* argv, bool allowUnregistered)
{
    lock_guard<mutex> lock(fMtx);
    // clear the container because it was filled with default values and subsequent calls to store() do not overwrite the existing values
    fVarMap.clear();

    if (allowUnregistered) {
        po::command_line_parser parser(argc, argv);
        parser.options(fAllOptions).allow_unregistered();
        po::parsed_options parsed = parser.run();
        fUnregisteredOptions = po::collect_unrecognized(parsed.options, po::include_positional);

        po::store(parsed, fVarMap);
    } else {
        po::store(po::parse_command_line(argc, argv, fAllOptions), fVarMap);
    }
}

void ProgOptions::Notify()
{
    lock_guard<mutex> lock(fMtx);
    po::notify(fVarMap);
}

void ProgOptions::AddToCmdLineOptions(const po::options_description optDesc, bool /* visible */)
{
    lock_guard<mutex> lock(fMtx);
    fAllOptions.add(optDesc);
}

po::options_description& ProgOptions::GetCmdLineOptions()
{
    return fAllOptions;
}

int ProgOptions::Count(const string& key) const
{
    lock_guard<mutex> lock(fMtx);
    return fVarMap.count(key);
}

unordered_map<string, int> ProgOptions::GetChannelInfo() const
{
    lock_guard<mutex> lock(fMtx);
    return GetChannelInfoImpl();
}

unordered_map<string, int> ProgOptions::GetChannelInfoImpl() const
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

vector<string> ProgOptions::GetPropertyKeys() const
{
    lock_guard<mutex> lock(fMtx);

    vector<string> keys;

    for (const auto& it : fVarMap) {
        keys.push_back(it.first.c_str());
    }

    return keys;
}

string ProgOptions::GetPropertyAsString(const string& key) const
{
    lock_guard<mutex> lock(fMtx);

    if (fVarMap.count(key)) {
        return ConvertVarValToString(fVarMap.at(key));
    } else {
        LOG(warn) << "Config has no key: " << key << ". Returning default constructed object.";
        return string();
    }
}

string ProgOptions::GetPropertyAsString(const string& key, const string& ifNotFound) const
{
    lock_guard<mutex> lock(fMtx);

    if (fVarMap.count(key)) {
        return ConvertVarValToString(fVarMap.at(key));
    }

    return ifNotFound;
}

Properties ProgOptions::GetProperties(const string& q) const
{
    boost::regex re(q);
    Properties properties;

    lock_guard<mutex> lock(fMtx);

    for (const auto& m : fVarMap) {
        if (boost::regex_match(m.first, re)) {
            properties.emplace(m.first, m.second.value());
        }
    }

    if (properties.size() == 0) {
        LOG(warn) << "could not find anything with \"" << q << "\"";
    }

    return properties;
}

Properties ProgOptions::GetPropertiesStartingWith(const string& q) const
{
    Properties properties;

    lock_guard<mutex> lock(fMtx);

    for (const auto& m : fVarMap) {
        if (m.first.compare(0, q.length(), q) == 0) {
            properties.emplace(m.first, m.second.value());
        }
    }

    return properties;
}

map<string, string> ProgOptions::GetPropertiesAsString(const string& q) const
{
    boost::regex re(q);
    map<string, string> properties;

    lock_guard<mutex> lock(fMtx);

    for (const auto& m : fVarMap) {
        if (boost::regex_match(m.first, re)) {
            properties.emplace(m.first, PropertyHelper::ConvertPropertyToString(m.second.value()));
        }
    }

    if (properties.size() == 0) {
        LOG(warn) << "could not find anything with \"" << q << "\"";
    }

    return properties;
}

map<string, string> ProgOptions::GetPropertiesAsStringStartingWith(const string& q) const
{
    map<string, string> properties;

    lock_guard<mutex> lock(fMtx);

    for (const auto& m : fVarMap) {
        if (m.first.compare(0, q.length(), q) == 0) {
            properties.emplace(m.first, PropertyHelper::ConvertPropertyToString(m.second.value()));
        }
    }

    return properties;
}

void ProgOptions::SetProperties(const Properties& input)
{
    unique_lock<mutex> lock(fMtx);

    map<string, boost::program_options::variable_value>& vm = fVarMap;
    for (const auto& m : input) {
        vm[m.first].value() = m.second;
    }

    lock.unlock();

    for (const auto& m : input) {
        PropertyHelper::fEventEmitters.at(m.second.type())(fEvents, m.first, m.second);
        fEvents.Emit<PropertyChangeAsString, string>(m.first, PropertyHelper::ConvertPropertyToString(m.second));
    }
}

bool ProgOptions::UpdateProperties(const Properties& input)
{
    unique_lock<mutex> lock(fMtx);

    for (const auto& m : input) {
        if (fVarMap.count(m.first) == 0) {
            LOG(debug) << "UpdateProperties failed, no property found with key '" << m.first << "'";
            return false;
        }
    }

    map<string, boost::program_options::variable_value>& vm = fVarMap;
    for (const auto& m : input) {
        vm[m.first].value() = m.second;
    }

    lock.unlock();

    for (const auto& m : input) {
        PropertyHelper::fEventEmitters.at(m.second.type())(fEvents, m.first, m.second);
        fEvents.Emit<PropertyChangeAsString, string>(m.first, PropertyHelper::ConvertPropertyToString(m.second));
    }

    return true;
}

void ProgOptions::DeleteProperty(const string& key)
{
    lock_guard<mutex> lock(fMtx);

    map<string, boost::program_options::variable_value>& vm = fVarMap;
    vm.erase(key);
}

void ProgOptions::AddChannel(const string& name, const FairMQChannel& channel)
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

void ProgOptions::PrintHelp() const
{
    cout << fAllOptions << endl;
}

void ProgOptions::PrintOptions() const
{
    map<string, ValInfo> mapinfo;

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
}

void ProgOptions::PrintOptionsRaw() const
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
}


} // namespace mq
} // namespace fair
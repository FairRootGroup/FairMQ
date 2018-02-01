/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/*
 * File:   FairProgOptions.cxx
 * Author: winckler
 *
 * Created on March 11, 2015, 5:38 PM
 */

#include "FairProgOptions.h"

#include <iomanip>
#include <sstream>
#include <algorithm>

using namespace std;
using namespace fair::mq;

FairProgOptions::FairProgOptions()
    : fVarMap()
    , fGeneralOptions("General options")
    , fAllOptions("Command line options")
    , fConfigMutex()
{
    fGeneralOptions.add_options()
        ("help,h", "produce help")
        ("version,v", "print version")
        ("severity", po::value<string>()->default_value("debug"), "Log severity level: trace, debug, info, state, warn, error, fatal, nolog")
        ("verbosity", po::value<string>()->default_value("medium"), "Log verbosity level: veryhigh, high, medium, low")
        ("color", po::value<bool>()->default_value(true), "Log color (true/false)")
        ("log-to-file", po::value<string>()->default_value(""), "Log output to a file.")
        ("print-options", po::value<bool>()->implicit_value(true), "print options in machine-readable format (<option>:<computed-value>:<type>:<description>)");

    fAllOptions.add(fGeneralOptions);
}

FairProgOptions::~FairProgOptions()
{
}

/// Add option descriptions
int FairProgOptions::AddToCmdLineOptions(const po::options_description optDesc, bool /* visible */)
{
    fAllOptions.add(optDesc);
    return 0;
}

po::options_description& FairProgOptions::GetCmdLineOptions()
{
    return fAllOptions;
}

int FairProgOptions::ParseCmdLine(const int argc, char const* const* argv, bool allowUnregistered)
{
    // get options from cmd line and store in variable map
    // here we use command_line_parser instead of parse_command_line, to allow unregistered and positional options
    if (allowUnregistered)
    {
        po::command_line_parser parser{argc, argv};
        parser.options(fAllOptions).allow_unregistered();
        po::parsed_options parsedOptions = parser.run();
        po::store(parsedOptions, fVarMap);
    }
    else
    {
        po::store(po::parse_command_line(argc, argv, fAllOptions), fVarMap);
    }

    // Handles options like "--help" or "--version"
    // return 1 if switch options found in fVarMap
    if (ImmediateOptions())
    {
        return 1;
    }

    po::notify(fVarMap);
    return 0;
}

void FairProgOptions::ParseDefaults()
{
    vector<string> emptyArgs;
    emptyArgs.push_back("dummy");

    vector<const char*> argv(emptyArgs.size());

    transform(emptyArgs.begin(), emptyArgs.end(), argv.begin(), [](const string& str)
    {
        return str.c_str();
    });

    po::store(po::parse_command_line(argv.size(), const_cast<char**>(argv.data()), fAllOptions), fVarMap);
}

int FairProgOptions::PrintOptions()
{
    // -> loop over variable map and print its content
    // -> In this example the following types are supported:
    // string, int, float, double, short, boost::filesystem::path
    // vector<string>, vector<int>, vector<float>, vector<double>, vector<short>

    map<string, VarValInfo> mapinfo;

    // get string length for formatting and convert varmap values into string
    int maxLen1st = 0;
    int maxLen2nd = 0;
    int maxLenTypeInfo = 0;
    int maxLenDefault = 0;
    int maxLenEmpty = 0;
    for (const auto& m : fVarMap)
    {
        Max(maxLen1st, m.first.length());

        VarValInfo valinfo = GetVariableValueInfo(m.second);
        mapinfo[m.first] = valinfo;

        Max(maxLen2nd, valinfo.value.length());
        Max(maxLenTypeInfo, valinfo.type.length());
        Max(maxLenDefault, valinfo.defaulted.length());
        Max(maxLenEmpty, valinfo.empty.length());
    }

    // TODO : limit the value len field in a better way
    if (maxLen2nd > 100)
    {
        maxLen2nd = 100;
    }

    stringstream ss;
    ss << "Configuration: \n";

    for (const auto& p : mapinfo)
    {
        ss << setfill(' ')
           << setw(maxLen1st) << left << p.first << " = "
           << setw(maxLen2nd)         << p.second.value
           << setw(maxLenTypeInfo)    << p.second.type
           << setw(maxLenDefault)     << p.second.defaulted
           << setw(maxLenEmpty)       << p.second.empty
           << "\n";
    }

    LOG(info) << ss.str();

    return 0;
}

int FairProgOptions::PrintOptionsRaw()
{
    const std::vector<boost::shared_ptr<po::option_description>>& options = fAllOptions.options();

    for (const auto& o : options)
    {
        VarValInfo value;
        if (fVarMap.count(o->canonical_display_name()))
        {
            value = GetVariableValueInfo(fVarMap[o->canonical_display_name()]);
        }

        string description = o->description();

        replace(description.begin(), description.end(), '\n', ' ');

        cout << o->long_name() << ":" << value.value << ":" << (value.type == "" ? "<unknown>" : value.type) << ":" << description << endl;
    }

    return 0;
}

VarValInfo FairProgOptions::GetVariableValueInfo(const po::variable_value& varValue)
{
    return ConvertVariableValue<ToVarValInfo>()(varValue);
}

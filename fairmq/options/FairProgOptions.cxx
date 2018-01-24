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

FairProgOptions::FairProgOptions()
    : fVarMap()
    , fGeneralDesc("General options")
    , fCmdLineOptions("Command line options")
    , fVisibleOptions("Visible options")
    , fConfigMutex()
{
    fGeneralDesc.add_options()
        ("help,h", "produce help")
        ("version,v", "print version")
        ("severity", po::value<string>()->default_value("debug"), "Log severity level : trace, debug, info, state, warn, error, fatal, nolog")
        ("verbosity", po::value<string>()->default_value("medium"), "Log verbosity level : veryhigh, high, medium, low")
        ("color", po::value<bool>()->default_value(true), "Log color (true/false)")
        ("print-options", po::value<bool>()->implicit_value(true), "print options in machine-readable format (<option>:<computed-value>:<type>:<description>)");
}

FairProgOptions::~FairProgOptions()
{
}

/// Add option descriptions
int FairProgOptions::AddToCmdLineOptions(const po::options_description optDesc, bool visible)
{
    fCmdLineOptions.add(optDesc);
    if (visible)
    {
        fVisibleOptions.add(optDesc);
    }
    return 0;
}

po::options_description& FairProgOptions::GetCmdLineOptions()
{
    return fCmdLineOptions;
}

int FairProgOptions::ParseCmdLine(const int argc, char const* const* argv, const po::options_description& desc, po::variables_map& varmap, bool allowUnregistered)
{
    // get options from cmd line and store in variable map
    // here we use command_line_parser instead of parse_command_line, to allow unregistered and positional options
    if (allowUnregistered)
    {
        po::command_line_parser parser{argc, argv};
        parser.options(desc).allow_unregistered();
        po::parsed_options parsedOptions = parser.run();
        po::store(parsedOptions, varmap);
    }
    else
    {
        po::store(po::parse_command_line(argc, argv, desc), varmap);
    }

    // Handles options like "--help" or "--version"
    // return 1 if switch options found in varmap
    if (ImmediateOptions())
    {
        return 1;
    }

    po::notify(varmap);
    return 0;
}

int FairProgOptions::ParseCmdLine(const int argc, char const* const* argv, const po::options_description& desc, bool allowUnregistered)
{
    return ParseCmdLine(argc, argv, desc, fVarMap, allowUnregistered);
}

void FairProgOptions::ParseDefaults(const po::options_description& desc)
{
    vector<string> emptyArgs;
    emptyArgs.push_back("dummy");

    vector<const char*> argv(emptyArgs.size());

    transform(emptyArgs.begin(), emptyArgs.end(), argv.begin(), [](const string& str)
    {
        return str.c_str();
    });

    po::store(po::parse_command_line(argv.size(), const_cast<char**>(argv.data()), desc), fVarMap);
}

int FairProgOptions::PrintOptions()
{
    // Method to overload.
    // -> loop over variable map and print its content
    // -> In this example the following types are supported:
    // string, int, float, double, short, boost::filesystem::path
    // vector<string>, vector<int>, vector<float>, vector<double>, vector<short>

    MapVarValInfo_t mapinfo;

    // get string length for formatting and convert varmap values into string
    int maxLength1st = 0;
    int maxLength2nd = 0;
    int maxLengthTypeInfo = 0;
    int maxLengthDefault = 0;
    int maxLengthEmpty = 0;
    int totalLength = 0;
    for (const auto& m : fVarMap)
    {
        Max(maxLength1st, m.first.length());

        VarValInfo_t valinfo = GetVariableValueInfo(m.second);
        mapinfo[m.first] = valinfo;
        string valueStr;
        string typeInfoStr;
        string defaultStr;
        string emptyStr;
        tie(valueStr, typeInfoStr, defaultStr, emptyStr) = valinfo;

        Max(maxLength2nd, valueStr.length());
        Max(maxLengthTypeInfo, typeInfoStr.length());
        Max(maxLengthDefault, defaultStr.length());
        Max(maxLengthEmpty, emptyStr.length());
    }

    // TODO : limit the value length field in a better way
    if (maxLength2nd > 100)
    {
        maxLength2nd = 100;
    }
    totalLength = maxLength1st + maxLength2nd + maxLengthTypeInfo + maxLengthDefault + maxLengthEmpty;

    // maxLength2nd = 200;

    // formatting and printing

    stringstream ss;
    ss << "\n";

    ss << setfill('*') << setw(totalLength + 3) << "*" << "\n"; // +3 because of string " = "
    string title = "     Configuration     ";

    int leftSpaceLength = 0;
    int rightSpaceLength = 0;
    int leftTitleShiftLength = 0;
    int rightTitleShiftLength = 0;

    leftTitleShiftLength = title.length() / 2;

    if ((title.length()) % 2)
    {
        rightTitleShiftLength = leftTitleShiftLength + 1;
    }
    else
    {
        rightTitleShiftLength = leftTitleShiftLength;
    }

    leftSpaceLength = (totalLength + 3) / 2 - leftTitleShiftLength;
    if ((totalLength + 3) % 2)
    {
        rightSpaceLength = (totalLength + 3) / 2 - rightTitleShiftLength + 1;
    }
    else
    {
        rightSpaceLength = (totalLength + 3) / 2 - rightTitleShiftLength;
    }

    ss << setfill ('*') << setw(leftSpaceLength) << "*"
       << setw(title.length()) << title
       << setfill ('*') << setw(rightSpaceLength) << "*" << "\n";

    ss << setfill ('*') << setw (totalLength+3) << "*" << "\n";

    for (const auto& p : mapinfo)
    {
        string keyStr;
        string valueStr;
        string typeInfoStr;
        string defaultStr;
        string emptyStr;
        keyStr = p.first;
        tie(valueStr, typeInfoStr, defaultStr, emptyStr) = p.second;
        ss << setfill(' ')
           << setw(maxLength1st) << left
           << p.first << " = "
           << setw(maxLength2nd)
           << valueStr
           << setw(maxLengthTypeInfo)
           << typeInfoStr
           << setw(maxLengthDefault)
           << defaultStr
           << setw(maxLengthEmpty)
           << emptyStr
           << "\n";
    }
    ss << setfill ('*') << setw(totalLength + 3) << "*";// +3 for " = "

    LOG(debug) << ss.str();

    return 0;
}

int FairProgOptions::PrintOptionsRaw()
{
    MapVarValInfo_t mapInfo;

    for (const auto& m : fVarMap)
    {
        mapInfo[m.first] = GetVariableValueInfo(m.second);
    }

    for (const auto& p : mapInfo)
    {
        string keyStr;
        string valueStr;
        string typeInfoStr;
        string defaultStr;
        string emptyStr;
        keyStr = p.first;
        tie(valueStr, typeInfoStr, defaultStr, emptyStr) = p.second;
        auto option = fCmdLineOptions.find_nothrow(keyStr, false);
        cout << keyStr << ":" << valueStr << ":" << typeInfoStr << ":" << (option ? option->description() : "<not found>") << endl;
    }

    return 0;
}

FairProgOptions::VarValInfo_t FairProgOptions::GetVariableValueInfo(const po::variable_value& varValue)
{
    return FairMQ::ConvertVariableValue<FairMQ::ToVarInfo>().Run(varValue);
}

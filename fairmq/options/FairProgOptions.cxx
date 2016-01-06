/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/*
 * File:   FairProgOptions.cxx
 * Author: winckler
 * 
 * Created on March 11, 2015, 5:38 PM
 */

#include "FairProgOptions.h"

using namespace std;

/// //////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constructor
FairProgOptions::FairProgOptions() :
                        fGenericDesc("Generic options description"),
                        fConfigDesc("Configuration options description"),
                        fHiddenDesc("Hidden options description"),
                        fEnvironmentDesc("Environment variables"),
                        fCmdLineOptions("Command line options"),
                        fConfigFileOptions("Configuration file options"),
                        fVisibleOptions("Visible options"),
                        fVerboseLvl("INFO"),
                        fUseConfigFile(false),
                        fConfigFile(),
                        fVarMap(),
                        fSeverityMap()
{
    // define generic options
    fGenericDesc.add_options()
        ("help,h", "produce help")
        ("version,v", "print version")
        ("verbose", po::value<std::string>(&fVerboseLvl)->default_value("DEBUG"), "Verbosity level : \n"
                                                                    "  TRACE \n"
                                                                    "  DEBUG \n"
                                                                    "  RESULTS \n"
                                                                    "  INFO \n"
                                                                    "  WARN \n"
                                                                    "  ERROR \n"
                                                                    "  STATE \n"
                                                                    "  NOLOG"
            )
        ("log-color", po::value<bool>()->default_value(true), "logger color: true or false")
        ;

    fSeverityMap["TRACE"]              = fairmq::severity_level::TRACE;
    fSeverityMap["DEBUG"]              = fairmq::severity_level::DEBUG;
    fSeverityMap["RESULTS"]            = fairmq::severity_level::RESULTS;
    fSeverityMap["INFO"]               = fairmq::severity_level::INFO;
    fSeverityMap["WARN"]               = fairmq::severity_level::WARN;
    fSeverityMap["ERROR"]              = fairmq::severity_level::ERROR;
    fSeverityMap["STATE"]              = fairmq::severity_level::STATE;
    fSeverityMap["NOLOG"]              = fairmq::severity_level::NOLOG;
}

/// Destructor
FairProgOptions::~FairProgOptions()
{
}

/// //////////////////////////////////////////////////////////////////////////////////////////////////////
/// Add option descriptions
int FairProgOptions::AddToCmdLineOptions(const po::options_description& optDesc, bool visible)
{
    fCmdLineOptions.add(optDesc);
    if (visible)
    {
        fVisibleOptions.add(optDesc);
    }
    return 0;
}

int FairProgOptions::AddToCfgFileOptions(const po::options_description& optDesc, bool visible)
{
    //if UseConfigFile() not yet called, then enable it with required file name to be provided by command line
    if (!fUseConfigFile)
    {
        UseConfigFile();
    }

    fConfigFileOptions.add(optDesc);
    if (visible)
    {
        fVisibleOptions.add(optDesc);
    }
    return 0;
}

int FairProgOptions::AddToEnvironmentOptions(const po::options_description& optDesc)
{
    fEnvironmentDesc.add(optDesc);
    return 0;
}

void FairProgOptions::UseConfigFile(const string& filename)
{
        fUseConfigFile = true;
        if (filename.empty())
        {
            fConfigDesc.add_options()
                ("config,c", po::value<boost::filesystem::path>(&fConfigFile)->required(), "Path to configuration file (required argument)");
            AddToCmdLineOptions(fConfigDesc);
        }
        else
        {
            fConfigFile = filename;
        }
}

/// //////////////////////////////////////////////////////////////////////////////////////////////////////
/// Parser

int FairProgOptions::ParseCmdLine(const int argc, char** argv, const po::options_description& desc, po::variables_map& varmap, bool allowUnregistered)
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

    // call the virtual NotifySwitchOption method to handle switch options like e.g. "--help" or "--version"
    // return 1 if switch options found in varmap
    if (NotifySwitchOption())
    {
        return 1;
    }

    po::notify(varmap);
    return 0;
}

int FairProgOptions::ParseCmdLine(const int argc, char** argv, const po::options_description& desc, bool allowUnregistered)
{
    return ParseCmdLine(argc, argv, desc, fVarMap, allowUnregistered);
}

int FairProgOptions::ParseCfgFile(ifstream& ifs, const po::options_description& desc, po::variables_map& varmap, bool allowUnregistered)
{
    if (!ifs)
    {
        cout << "can not open configuration file \n";
        return -1;
    }
    else
    {
        po:store(parse_config_file(ifs, desc, allowUnregistered), varmap);
        po::notify(varmap);
    }
    return 0;
}

int FairProgOptions::ParseCfgFile(const string& filename, const po::options_description& desc, po::variables_map& varmap, bool allowUnregistered)
{
    ifstream ifs(filename.c_str());
    if (!ifs)
    {
        cout << "can not open configuration file: " << filename << "\n";
        return -1;
    }
    else
    {
        po:store(parse_config_file(ifs, desc, allowUnregistered), varmap);
        po::notify(varmap);
    }
    return 0;
}

int FairProgOptions::ParseCfgFile(const string& filename, const po::options_description& desc, bool allowUnregistered)
{
    return ParseCfgFile(filename,desc,fVarMap,allowUnregistered);
}

int FairProgOptions::ParseCfgFile(ifstream& ifs, const po::options_description& desc, bool allowUnregistered)
{
    return ParseCfgFile(ifs,desc,fVarMap,allowUnregistered);
}

int FairProgOptions::ParseEnvironment(const function<string(string)>& environmentMapper)
{
    po::store(po::parse_environment(fEnvironmentDesc, environmentMapper), fVarMap);
    po::notify(fVarMap);

    return 0;
}

// Given a key, convert the variable value to string
string FairProgOptions::GetStringValue(const string& key)
{
    string valueStr;
    try
    {
        if (fVarMap.count(key))
        {
            valueStr=fairmq::ConvertVariableValue<fairmq::ToString>().Run(fVarMap.at(key));
        }
    }
    catch(exception& e)
    {
        LOG(ERROR) << "Exception thrown for the key '" << key << "'";
        LOG(ERROR) << e.what();
    }

    return valueStr;
}

/// //////////////////////////////////////////////////////////////////////////////////////////////////////
/// Print/notify options
int FairProgOptions::PrintHelp()  const
{
    cout << fVisibleOptions << "\n";
    return 0;
}

int FairProgOptions::PrintOptions()
{
    // //////////////////////////////////
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

    LOG(INFO) << setfill ('*') << setw (totalLength + 3) << "*";// +3 because of string " = "
    string PrintOptionsTitle = "     Program options found     ";

    int leftSpaceLength = 0;
    int rightSpaceLength = 0;
    int leftTitleShiftLength = 0;
    int rightTitleShiftLength = 0;

    leftTitleShiftLength = PrintOptionsTitle.length() / 2;

    if ((PrintOptionsTitle.length()) % 2)
        rightTitleShiftLength = leftTitleShiftLength + 1;
    else
        rightTitleShiftLength = leftTitleShiftLength;

    leftSpaceLength = (totalLength + 3) / 2 - leftTitleShiftLength;
    if ((totalLength + 3) % 2)
    {
        rightSpaceLength = (totalLength + 3) / 2 - rightTitleShiftLength + 1;
    }
    else
    {
        rightSpaceLength = (totalLength + 3) / 2 - rightTitleShiftLength;
    }

    LOG(INFO) << setfill ('*') << setw(leftSpaceLength) << "*"
                << setw(PrintOptionsTitle.length()) << PrintOptionsTitle
                << setfill ('*') << setw(rightSpaceLength) << "*";

    LOG(INFO) << setfill ('*') << setw (totalLength+3) << "*";

    for (const auto& p : mapinfo)
    {
        string keyStr;
        string valueStr;
        string typeInfoStr;
        string defaultStr;
        string emptyStr;
        keyStr = p.first;
        tie(valueStr, typeInfoStr, defaultStr, emptyStr) = p.second;
        LOG(INFO) << std::setfill(' ')
                    << setw(maxLength1st) << left
                    << p.first << " = "
                    << setw(maxLength2nd)
                    << valueStr
                    << setw(maxLengthTypeInfo)
                    << typeInfoStr
                    << setw(maxLengthDefault)
                    << defaultStr
                    << setw(maxLengthEmpty)
                    << emptyStr;
    }
    LOG(INFO) << setfill ('*') << setw (totalLength + 3) << "*";// +3 for " = "

    return 0;
}

int FairProgOptions::NotifySwitchOption()
{
    // Method to overload.
    if (fVarMap.count("help"))
    {
        cout << "***** FAIR Program Options ***** \n" << fVisibleOptions;
        return 1;
    }

    if (fVarMap.count("version"))
    {
        cout << "alpha version 0.0\n";
        return 1;
    }

    return 0;
}

FairProgOptions::VarValInfo_t FairProgOptions::GetVariableValueInfo(const po::variable_value& varValue)
{
    return fairmq::ConvertVariableValue<fairmq::ToVarInfo>().Run(varValue);
}

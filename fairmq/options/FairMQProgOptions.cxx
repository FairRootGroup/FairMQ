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

FairMQProgOptions::FairMQProgOptions()
    : FairProgOptions()
    , fMQParserOptions("MQ-Device parser options")
    , fMQtree()
    , fFairMQmap()
{
    InitOptionDescription();
}

FairMQProgOptions::~FairMQProgOptions() 
{
}

int FairMQProgOptions::ParseAll(const int argc, char** argv, bool AllowUnregistered)
{
    // before parsing, define cmdline and optionally cfgfile description, 
    // and also what is visible for the user
    AddToCmdLineOptions(fGenericDesc);
    AddToCmdLineOptions(fMQParserOptions);

    // if config file option enabled then id non required in cmdline but required in configfile
    // else required in cmdline
    if (fUseConfigFile)
    {
        fCmdline_options.add_options()
            ("device-id", po::value< std::string >(), "Device ID");
        fConfig_file_options.add_options()
            ("device-id", po::value< std::string >()->required(), "Device ID");
    }
    else
    {
        fCmdline_options.add_options()
            ("device-id", po::value< std::string >()->required(), "Device ID");
    }

    fVisible_options.add_options()
            ("device-id", po::value< std::string >()->required(), "Device ID (required value)");

    // parse command line
    if (ParseCmdLine(argc,argv,fCmdline_options,fvarmap,AllowUnregistered))
    {
        return 1;
    }

    // if txt/INI configuration file enabled then parse it
    if (fUseConfigFile && !fConfigFile.empty())
    {
        AddToCfgFileOptions(fMQParserOptions,false);

        if (ParseCfgFile(fConfigFile, fConfig_file_options, fvarmap, AllowUnregistered))
        {
            return 1;
        }
    }

    // set log level before printing (default is 0 = DEBUG level)
    int verbose=GetValue<int>("verbose");
    SET_LOGGER_LEVEL(verbose);

    PrintOptions();

    return 0;
}

int FairMQProgOptions::NotifySwitchOption()
{
    if ( fvarmap.count("help") )
    {
        std::cout << "***** FAIRMQ Program Options ***** \n" << fVisible_options;
        return 1;
    }

    if (fvarmap.count("version")) 
    {
        std::cout << "Beta version 0.1\n";
        return 1;
    }

    return 0;
}


void FairMQProgOptions::InitOptionDescription()
{
    fMQParserOptions.add_options()
        ("config-xml-string",     po::value< std::vector<std::string> >()->multitoken(), "XML input as command line string.")
        ("config-xml-filename",   po::value< std::string >(), "XML input as file.")
        ("config-json-string",    po::value< std::vector<std::string> >()->multitoken(), "JSON input as command line string.")
        ("config-json-filename",  po::value< std::string >(), "JSON input as file.")
        
        // ("ini.config.string",     po::value< std::vector<std::string> >()->multitoken(), "INI input as command line string.")
        // ("ini.config.filename",   po::value< std::string >(), "INI input as file.")
    ;
}
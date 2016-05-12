/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

/*
 * File:   FairMQProgOptions.h
 * Author: winckler
 *
 * Created on March 11, 2015, 10:20 PM
 */

#ifndef FAIRMQPROGOPTIONS_H
#define FAIRMQPROGOPTIONS_H

#include <unordered_map>

#include "FairProgOptions.h"

#include "FairMQChannel.h"
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

class FairMQProgOptions : public FairProgOptions
{
  protected:
    typedef std::unordered_map<std::string, std::vector<FairMQChannel>> FairMQMap;

  public:
    FairMQProgOptions();
    virtual ~FairMQProgOptions();

    // parse command line and txt/INI configuration file. 
    // default parser for the mq-configuration file (JSON/XML) is called if command line key mq-config is called
    virtual void ParseAll(const int argc, char** argv, bool allowUnregistered = false);

    // external parser, store function 
    template <typename T, typename ...Args>
    int UserParser(Args &&... args)
    {
        try
        {
            Store(T().UserParser(std::forward<Args>(args)...));
        }
        catch (std::exception& e)
        {
            LOG(ERROR) << e.what();
            return 1;
        }
        return 0;
    }

    int Store(const po::variables_map& vm)
    {
        fVarMap = vm;
        return 0;
    }

    int Store(const pt::ptree& tree)
    {
        fMQtree = tree;
        return 0;
    }

    int Store(const FairMQMap& channels)
    {
        fFairMQMap = channels;
        return 0;
    }

    FairMQMap GetFairMQMap()
    {
        return fFairMQMap;
    }

    // to customize title of the executable help command line  
    void SetHelpTitle(const std::string& title)
    {
        fHelpTitle=title;
    }
    // to customize the executable version command line
    void SetVersion(const std::string& version)
    {
        fVersion=version;
    }

  protected:
    po::options_description fMQParserOptions;
    po::options_description fMQOptionsInCfg;
    po::options_description fMQOptionsInCmd;
    pt::ptree fMQtree;
    FairMQMap fFairMQMap;
    std::string fHelpTitle;
    std::string fVersion;

    virtual int NotifySwitchOption(); // for custom help & version printing
    void InitOptionDescription();
};


#endif /* FAIRMQPROGOPTIONS_H */


/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/*
 * File:   FairProgOptions.h
 * Author: winckler
 *
 * Created on March 11, 2015, 5:38 PM
 */

#ifndef FAIRPROGOPTIONS_H
#define FAIRPROGOPTIONS_H

#include "FairMQLogger.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "FairProgOptionsHelper.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <iterator>
#include <mutex>
#include <tuple>

/*
 * FairProgOptions abstract base class
 * parse command line, configuration file options as well as environment variables.
 * 
 * The user defines in the derived class the option descriptions and
 * the pure virtual ParseAll() method
 * 
 * class MyOptions : public FairProgOptions
 * {
 *      public : 
 *      MyOptions() : FairProgOptions()
 *      {
 *          fCmdlineOptions.add(fGenericDesc);
 *          fVisibleOptions.add(fCmdlineOptions);
 *      }
 *      virtual ~MyOptions() {}
 *      virtual void ParseAll(const int argc, char** argv, bool allowUnregistered = false)
 *      {
 *          if (ParseCmdLine(argc, argv, fCmdlineOptions, fVarMap, allowUnregistered))
 *          {
 *              exit(EXIT_FAILURE);
 *          }
 *
 *          PrintOptions();
 *      }
 * }
 */

namespace po = boost::program_options;
namespace fs = boost::filesystem;

class FairProgOptions
{
  public:
    FairProgOptions();
    virtual ~FairProgOptions();

    //  add options_description
    int AddToCmdLineOptions(const po::options_description& optDesc, bool visible = true);
    int AddToCfgFileOptions(const po::options_description& optDesc, bool visible = true);
    int AddToEnvironmentOptions(const po::options_description& optDesc);
    po::options_description& GetCmdLineOptions();
    po::options_description& GetCfgFileOptions();
    po::options_description& GetEnvironmentOptions();

    void UseConfigFile(const std::string& filename = "");

    // get value corresponding to the key
    template<typename T>
    T GetValue(const std::string& key) const
    {
        std::unique_lock<std::mutex> lock(fConfigMutex);

        T val = T();
        try
        {
            if (fVarMap.count(key))
            {
                val = fVarMap[key].as<T>();
            }
            else
            {
                LOG(ERROR) << "Config has no key: " << key;
            }
        }
        catch (std::exception& e)
        {
            LOG(ERROR) << "Exception thrown for the key '" << key << "'";
            LOG(ERROR) << e.what();
            this->PrintHelp();
        }

        return val;
    }

    // Given a key, convert the variable value to string
    std::string GetStringValue(const std::string& key)
    {
        std::unique_lock<std::mutex> lock(fConfigMutex);

        std::string valueStr;
        try
        {
            if (fVarMap.count(key))
            {
                valueStr = FairMQ::ConvertVariableValue<FairMQ::ToString>().Run(fVarMap.at(key));
            }
        }
        catch (std::exception& e)
        {
            LOG(ERROR) << "Exception thrown for the key '" << key << "'";
            LOG(ERROR) << e.what();
        }

        return valueStr;
    }

    int Count(const std::string& key) const
    {
        std::unique_lock<std::mutex> lock(fConfigMutex);

        return fVarMap.count(key);
    }

    //restrict conversion to fundamental types
    template<typename T>
    T ConvertTo(const std::string& strValue)
    {
        if (std::is_arithmetic<T>::value) 
        {
            std::istringstream iss(strValue);
            T val;
            iss >> val;
            return val;
        } 
        else 
        {
            LOG(ERROR) << "the provided string " << strValue << " cannot be converted in the requested type. The target types must be arithmetic types";
        }
    }

    const po::variables_map& GetVarMap() const { return fVarMap; }

    // boost prog options parsers
    int ParseCmdLine(const int argc, const char** argv, const po::options_description& desc, po::variables_map& varmap, bool allowUnregistered = false);
    int ParseCmdLine(const int argc, const char** argv, const po::options_description& desc, bool allowUnregistered = false);

    int ParseCfgFile(const std::string& filename, const po::options_description& desc, po::variables_map& varmap, bool allowUnregistered = false);
    int ParseCfgFile(const std::string& filename, const po::options_description& desc, bool allowUnregistered = false);
    int ParseCfgFile(std::ifstream& ifs, const po::options_description& desc, po::variables_map& varmap, bool allowUnregistered = false);
    int ParseCfgFile(std::ifstream& ifs, const po::options_description& desc, bool allowUnregistered = false);

    int ParseEnvironment(const std::function<std::string(std::string)>&);

    virtual void ParseAll(const int argc, const char** argv, bool allowUnregistered = false) = 0;// TODO change return type to bool and propagate to executable

    virtual int PrintOptions();
    virtual int PrintOptionsRaw();
    int PrintHelp() const;

  protected:
    // options container
    po::variables_map fVarMap;

    // basic description categories
    po::options_description fGenericDesc;
    po::options_description fConfigDesc;
    po::options_description fEnvironmentDesc;
    po::options_description fHiddenDesc;

    // Description of cmd line and simple configuration file (configuration file like txt, but not like xml json ini)
    po::options_description fCmdLineOptions;
    po::options_description fConfigFileOptions;

    // Description which is printed in help command line
    // to handle logger severity
    std::map<std::string, FairMQ::severity_level> fSeverityMap;
    po::options_description fVisibleOptions;

    mutable std::mutex fConfigMutex;

    std::string fVerbosityLevel;
    bool fUseConfigFile;
    boost::filesystem::path fConfigFile;
    virtual int NotifySwitchOption();

    // UpdateVarMap() and replace() --> helper functions to modify the value of variable map after calling po::store
    template<typename T>
    void UpdateVarMap(const std::string& key, const T& val)
    {
        replace(fVarMap, key, val);
    }

    template<typename T>
    void replace(std::map<std::string, po::variable_value>& vm, const std::string& opt, const T& val)
    {
        vm[opt].value() = boost::any(val);
    }

  private:
    // Methods below are helper functions used in the PrintOptions method
    typedef std::tuple<std::string, std::string, std::string, std::string> VarValInfo_t;
    typedef std::map<std::string, VarValInfo_t> MapVarValInfo_t;

    VarValInfo_t GetVariableValueInfo(const po::variable_value& varValue);

    static void Max(int &val, const int &comp)
    {
        if (comp > val)
        {
            val = comp;
        }
    }
};

#endif /* FAIRPROGOPTIONS_H */

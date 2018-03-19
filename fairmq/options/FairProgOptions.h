/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
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
#include "FairProgOptionsHelper.h"
#include <fairmq/Tools.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <mutex>
#include <exception>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

class FairProgOptions
{
  public:
    FairProgOptions();
    virtual ~FairProgOptions();

    auto GetPropertyKeys() const -> std::vector<std::string>
    {
        std::lock_guard<std::mutex> lock{fConfigMutex};

        std::vector<std::string> result;

        for (const auto& it : fVarMap)
        {
            result.push_back(it.first.c_str());
        }

        return result;
    }

    //  add options_description
    int AddToCmdLineOptions(const po::options_description optDesc, bool visible = true);
    po::options_description& GetCmdLineOptions();

    // get value corresponding to the key
    template<typename T>
    T GetValue(const std::string& key) const
    {
        std::unique_lock<std::mutex> lock(fConfigMutex);

        T val = T();

        if (fVarMap.count(key))
        {
            val = fVarMap[key].as<T>();
        }
        else
        {
            LOG(warn) << "Config has no key: " << key << ". Returning default constructed object.";
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
                valueStr = fair::mq::ConvertVariableValue<fair::mq::VarInfoToString>()(fVarMap.at(key));
            }
        }
        catch (std::exception& e)
        {
            LOG(error) << "Exception thrown for the key '" << key << "'";
            LOG(error) << e.what();
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
            LOG(error) << "the provided string " << strValue << " cannot be converted in the requested type. The target types must be arithmetic types";
        }
    }

    const po::variables_map& GetVarMap() const { return fVarMap; }

    int ParseCmdLine(const int argc, char const* const* argv, bool allowUnregistered = false);
    void ParseDefaults();

    virtual int ParseAll(const int argc, char const* const* argv, bool allowUnregistered = false) = 0;

    virtual int PrintOptions();
    virtual int PrintOptionsRaw();

  protected:
    // options container
    po::variables_map fVarMap;

    // options descriptions
    po::options_description fGeneralOptions;
    po::options_description fAllOptions;

    mutable std::mutex fConfigMutex;

    virtual int ImmediateOptions() = 0;

    // UpdateVarMap() and Replace() --> helper functions to modify the value of variable map after calling po::store
    template<typename T>
    void UpdateVarMap(const std::string& key, const T& val)
    {
        Replace(fVarMap, key, val);
    }

    template<typename T>
    void Replace(std::map<std::string, po::variable_value>& vm, const std::string& key, const T& val)
    {
        vm[key].value() = boost::any(val);
    }

  private:
    fair::mq::VarValInfo GetVariableValueInfo(const po::variable_value& varValue);

    static void Max(int& val, const int& comp)
    {
        if (comp > val)
        {
            val = comp;
        }
    }
};

#endif /* FAIRPROGOPTIONS_H */

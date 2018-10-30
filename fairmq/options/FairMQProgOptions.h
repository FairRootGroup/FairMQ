/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQPROGOPTIONS_H
#define FAIRMQPROGOPTIONS_H

#include <fairmq/EventManager.h>
#include "FairMQLogger.h"
#include "FairMQChannel.h"
#include <fairmq/Tools.h>

#include <boost/program_options.hpp>

#include <unordered_map>
#include <functional>
#include <string>
#include <vector>
#include <mutex>
#include <sstream>

namespace fair
{
namespace mq
{

struct PropertyChange : Event<std::string> {};
struct PropertyChangeAsString : Event<std::string> {};

} /* namespace mq */
} /* namespace fair */

class FairMQProgOptions
{
  private:
    using FairMQChannelMap = std::unordered_map<std::string, std::vector<FairMQChannel>>;

  public:
    FairMQProgOptions();
    virtual ~FairMQProgOptions();

    int ParseAll(const std::vector<std::string>& cmdLineArgs, bool allowUnregistered);
    // parse command line.
    // default parser for the mq-configuration file (JSON) is called if command line key mq-config is called
    int ParseAll(const int argc, char const* const* argv, bool allowUnregistered = true);

    FairMQChannelMap GetFairMQMap() const;
    std::unordered_map<std::string, int> GetChannelInfo() const;

    template<typename T>
    int SetValue(const std::string& key, T val)
    {
        std::unique_lock<std::mutex> lock(fConfigMutex);

        // update variable map
        UpdateVarMap<typename std::decay<T>::type>(key, val);

        if (key == "channel-config")
        {
            ParseChannelsFromCmdLine();
        }
        else if (fChannelKeyMap.count(key))
        {
            UpdateChannelValue(fChannelKeyMap.at(key).channel, fChannelKeyMap.at(key).index, fChannelKeyMap.at(key).member, val);
        }

        lock.unlock();

        //if (std::is_same<T, int>::value || std::is_same<T, std::string>::value)//if one wants to restrict type
        fEvents.Emit<fair::mq::PropertyChange, typename std::decay<T>::type>(key, val);
        fEvents.Emit<fair::mq::PropertyChangeAsString, std::string>(key, GetStringValue(key));

        return 0;
    }

    template <typename T>
    void Subscribe(const std::string& subscriber, std::function<void(typename fair::mq::PropertyChange::KeyType, T)> func)
    {
        std::unique_lock<std::mutex> lock(fConfigMutex);

        static_assert(!std::is_same<T,const char*>::value || !std::is_same<T, char*>::value,
            "In template member FairMQProgOptions::Subscribe<T>(key,Lambda) the types const char* or char* for the calback signatures are not supported.");

        fEvents.Subscribe<fair::mq::PropertyChange, T>(subscriber, func);
    }

    template <typename T>
    void Unsubscribe(const std::string& subscriber)
    {
        std::unique_lock<std::mutex> lock(fConfigMutex);

        fEvents.Unsubscribe<fair::mq::PropertyChange, T>(subscriber);
    }

    void SubscribeAsString(const std::string& subscriber, std::function<void(typename fair::mq::PropertyChange::KeyType, std::string)> func)
    {
        std::unique_lock<std::mutex> lock(fConfigMutex);

        fEvents.Subscribe<fair::mq::PropertyChangeAsString, std::string>(subscriber, func);
    }

    void UnsubscribeAsString(const std::string& subscriber)
    {
        std::unique_lock<std::mutex> lock(fConfigMutex);

        fEvents.Unsubscribe<fair::mq::PropertyChangeAsString, std::string>(subscriber);
    }

    std::vector<std::string> GetPropertyKeys() const;

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
    std::string GetStringValue(const std::string& key);

    int Count(const std::string& key) const;

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
            LOG(error) << "the provided string " << strValue << " cannot be converted to the requested type. The target type must be arithmetic type.";
        }
    }

    //  add options_description
    int AddToCmdLineOptions(const boost::program_options::options_description optDesc, bool visible = true);
    boost::program_options::options_description& GetCmdLineOptions();

    const boost::program_options::variables_map& GetVarMap() const { return fVarMap; }

    int PrintOptions();
    int PrintOptionsRaw();

    void AddChannel(const std::string& channelName, const FairMQChannel& channel)
    {
        fFairMQChannelMap[channelName].push_back(channel);
    }

  private:
    struct ChannelKey
    {
        std::string channel;
        int index;
        std::string member;
    };

    boost::program_options::variables_map fVarMap; ///< options container
    FairMQChannelMap fFairMQChannelMap;

    boost::program_options::options_description fAllOptions; ///< all options descriptions
    boost::program_options::options_description fGeneralOptions; ///< general options descriptions
    boost::program_options::options_description fMQOptions; ///< MQ options descriptions
    boost::program_options::options_description fParserOptions; ///< MQ Parser options descriptions

    mutable std::mutex fConfigMutex;

    std::unordered_map<std::string, int> fChannelInfo; ///< channel name - number of subchannels
    std::unordered_map<std::string, ChannelKey> fChannelKeyMap;// key=full path - val=key info
    std::vector<std::string> fUnregisteredOptions; ///< container with unregistered options

    fair::mq::EventManager fEvents;

    void ParseCmdLine(const int argc, char const* const* argv, bool allowUnregistered = true);
    void ParseDefaults();

    // read FairMQChannelMap and insert/update corresponding values in variable map
    // create key for variable map as follow : channelName.index.memberName
    void UpdateMQValues();
    int Store(const FairMQChannelMap& channels);

    template<typename T>
    void EmitUpdate(const std::string& key, T val)
    {
        // compile time check whether T is const char* or char*, and in that case a compile time error is thrown.
        static_assert(!std::is_same<T,const char*>::value || !std::is_same<T, char*>::value,
            "In template member FairMQProgOptions::EmitUpdate<T>(key,val) the types const char* or char* for the calback signatures are not supported.");
        fEvents.Emit<fair::mq::PropertyChange, T>(key, val);
        fEvents.Emit<fair::mq::PropertyChangeAsString, std::string>(key, GetStringValue(key));
    }

    int UpdateChannelMap(const FairMQChannelMap& map);
    template<typename T>
    int UpdateChannelValue(const std::string&, int, const std::string&, T)
    {
        LOG(error)  << "update of FairMQChannel map failed, because value type not supported";
        return 1;
    }
    int UpdateChannelValue(const std::string& channelName, int index, const std::string& member, const std::string& val);
    int UpdateChannelValue(const std::string& channelName, int index, const std::string& member, int val);

    void UpdateChannelInfo();

    // helper to modify the value of variable map after calling boost::program_options::store
    template<typename T>
    void UpdateVarMap(const std::string& key, const T& val)
    {
        std::map<std::string, boost::program_options::variable_value>& vm = fVarMap;
        vm[key].value() = boost::any(val);
    }

    void ParseChannelsFromCmdLine();
};

#endif /* FAIRMQPROGOPTIONS_H */

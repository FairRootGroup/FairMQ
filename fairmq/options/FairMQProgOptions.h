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
#include <map>
#include <functional>
#include <string>
#include <vector>
#include <mutex>
#include <regex>
#include <sstream>
#include <typeindex>
#include <stdexcept>

namespace fair
{
namespace mq
{

struct PropertyChange : Event<std::string> {};
struct PropertyChangeAsString : Event<std::string> {};

struct ValInfo
{
    std::string value;
    std::string type;
    std::string origin;
};

} /* namespace mq */
} /* namespace fair */

class FairMQProgOptions
{
  private:
    using FairMQChannelMap = std::unordered_map<std::string, std::vector<FairMQChannel>>;

  public:
    FairMQProgOptions();
    virtual ~FairMQProgOptions() {}

    struct PropertyNotFoundException : std::runtime_error { using std::runtime_error::runtime_error; };

    int ParseAll(const std::vector<std::string>& cmdArgs, bool allowUnregistered)
    {
        std::vector<const char*> argv(cmdArgs.size());
        transform(cmdArgs.begin(), cmdArgs.end(), argv.begin(), [](const std::string& str) {
            return str.c_str();
        });
        return ParseAll(argv.size(), const_cast<char**>(argv.data()), allowUnregistered);
    }

    int ParseAll(const int argc, char const* const* argv, bool allowUnregistered = true);

    FairMQChannelMap GetFairMQMap() const;
    std::unordered_map<std::string, int> GetChannelInfo() const;
    std::vector<std::string> GetPropertyKeys() const;

    template<typename T>
    T GetProperty(const std::string& key) const
    {
        std::lock_guard<std::mutex> lock(fMtx);

        if (fVarMap.count(key)) {
            return fVarMap[key].as<T>();
        }

       throw PropertyNotFoundException(fair::mq::tools::ToString("Config has no key: ", key));
    }

    template<typename T>
    T GetProperty(const std::string& key, const T& ifNotFound) const
    {
        std::lock_guard<std::mutex> lock(fMtx);

        if (fVarMap.count(key)) {
            return fVarMap[key].as<T>();
        }

        return ifNotFound;
    }

    template<typename T>
    T GetValue(const std::string& key) const // TODO: deprecate this
    {
        return GetProperty<T>(key);
    }

    std::map<std::string, boost::any> GetProperties(const std::string& q)
    {
        std::regex re(q);
        std::map<std::string, boost::any> result;

        std::lock_guard<std::mutex> lock(fMtx);

        for (const auto& m : fVarMap) {
            if (std::regex_search(m.first, re)) {
                result.emplace(m.first, m.second.value());
            }
        }

        return result;
    }

    std::string GetStringValue(const std::string& key);

    template<typename T>
    void SetProperty(const std::string& key, T val)
    {
        std::unique_lock<std::mutex> lock(fMtx);

        SetVarMapValue<typename std::decay<T>::type>(key, val);

        if (key == "channel-config") {
            ParseChannelsFromCmdLine();
        } else if (fChannelKeyMap.count(key)) {
            UpdateChannelValue(fChannelKeyMap.at(key).channel, fChannelKeyMap.at(key).index, fChannelKeyMap.at(key).member, val);
        }

        lock.unlock();

        fEvents.Emit<fair::mq::PropertyChange, typename std::decay<T>::type>(key, val);
        fEvents.Emit<fair::mq::PropertyChangeAsString, std::string>(key, GetStringValue(key));
    }

    template<typename T>
    int SetValue(const std::string& key, T val) // TODO: deprecate this
    {
        SetProperty(key, val);
        return 0;
    }

    void SetProperties(const std::map<std::string, boost::any>& input)
    {
        std::lock_guard<std::mutex> lock(fMtx);

        std::map<std::string, boost::program_options::variable_value>& vm = fVarMap;
        for (const auto& m : input) {
            vm[m.first].value() = m.second;
        }

        // TODO: call subscriptions here (after unlock)
    }

    void DeleteProperty(const std::string& key)
    {
        std::lock_guard<std::mutex> lock(fMtx);

        std::map<std::string, boost::program_options::variable_value>& vm = fVarMap;
        vm.erase(key);
    }

    template <typename T>
    void Subscribe(const std::string& subscriber, std::function<void(typename fair::mq::PropertyChange::KeyType, T)> func)
    {
        std::lock_guard<std::mutex> lock(fMtx);
        static_assert(!std::is_same<T,const char*>::value || !std::is_same<T, char*>::value,
            "In template member FairMQProgOptions::Subscribe<T>(key,Lambda) the types const char* or char* for the calback signatures are not supported.");
        fEvents.Subscribe<fair::mq::PropertyChange, T>(subscriber, func);
    }

    template <typename T>
    void Unsubscribe(const std::string& subscriber)
    {
        std::lock_guard<std::mutex> lock(fMtx);
        fEvents.Unsubscribe<fair::mq::PropertyChange, T>(subscriber);
    }

    void SubscribeAsString(const std::string& subscriber, std::function<void(typename fair::mq::PropertyChange::KeyType, std::string)> func)
    {
        std::lock_guard<std::mutex> lock(fMtx);
        fEvents.Subscribe<fair::mq::PropertyChangeAsString, std::string>(subscriber, func);
    }

    void UnsubscribeAsString(const std::string& subscriber)
    {
        std::lock_guard<std::mutex> lock(fMtx);
        fEvents.Unsubscribe<fair::mq::PropertyChangeAsString, std::string>(subscriber);
    }

    int Count(const std::string& key) const;

    //  add options_description
    int AddToCmdLineOptions(const boost::program_options::options_description optDesc, bool visible = true);
    boost::program_options::options_description& GetCmdLineOptions();

    int PrintOptions();
    int PrintOptionsRaw();

    void AddChannel(const std::string& channelName, const FairMQChannel& channel)
    {
        fFairMQChannelMap[channelName].push_back(channel);
    }

    static std::unordered_map<std::type_index, std::pair<std::string, std::string>(*)(const boost::any&)> fValInfos;

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

    mutable std::mutex fMtx;

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

    int UpdateChannelMap(const FairMQChannelMap& map);
    template<typename T>
    int UpdateChannelValue(const std::string&, int, const std::string&, T)
    {
        LOG(error)  << "update of FairMQChannel map failed, because value type not supported";
        return 1;
    }
    int UpdateChannelValue(const std::string& channelName, int index, const std::string& member, const std::string& val);
    int UpdateChannelValue(const std::string& channelName, int index, const std::string& member, int val);
    int UpdateChannelValue(const std::string& channelName, int index, const std::string& member, bool val);

    void UpdateChannelInfo();

    // modify the value of variable map after calling boost::program_options::store
    template<typename T>
    void SetVarMapValue(const std::string& key, const T& val)
    {
        std::map<std::string, boost::program_options::variable_value>& vm = fVarMap;
        vm[key].value() = boost::any(val);
    }

    void ParseChannelsFromCmdLine();
};

#endif /* FAIRMQPROGOPTIONS_H */

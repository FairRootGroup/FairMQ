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
#include "Properties.h"
#include <fairmq/Tools.h>

#include <boost/program_options.hpp>
#include <boost/core/demangle.hpp>

#include <unordered_map>
#include <map>
#include <functional>
#include <string>
#include <vector>
#include <mutex>
#include <sstream>
#include <typeindex>
#include <typeinfo>
#include <utility> // pair
#include <stdexcept>

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
  public:
    FairMQProgOptions();
    virtual ~FairMQProgOptions() {}

    struct PropertyNotFoundException : std::runtime_error { using std::runtime_error::runtime_error; };

    int ParseAll(const std::vector<std::string>& cmdArgs, bool allowUnregistered);
    int ParseAll(const int argc, char const* const* argv, bool allowUnregistered = true);

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

    fair::mq::Properties GetProperties(const std::string& q) const;
    std::map<std::string, std::string> GetPropertiesAsString(const std::string& q) const;
    fair::mq::Properties GetPropertiesStartingWith(const std::string& q) const;

    template<typename T>
    T GetValue(const std::string& key) const // TODO: deprecate this
    {
        return GetProperty<T>(key);
    }

    std::string GetPropertyAsString(const std::string& key) const;

    std::string GetStringValue(const std::string& key) const // TODO: deprecate this
    {
        return GetPropertyAsString(key);
    }

    template<typename T>
    void SetProperty(const std::string& key, T val)
    {
        std::unique_lock<std::mutex> lock(fMtx);

        SetVarMapValue<typename std::decay<T>::type>(key, val);

        if (key == "channel-config") {
            ParseChannelsFromCmdLine();
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

    void SetProperties(const fair::mq::Properties& input);
    void DeleteProperty(const std::string& key);

    template<typename T>
    void Subscribe(const std::string& subscriber, std::function<void(typename fair::mq::PropertyChange::KeyType, T)> func) const
    {
        std::lock_guard<std::mutex> lock(fMtx);
        static_assert(!std::is_same<T,const char*>::value || !std::is_same<T, char*>::value,
            "In template member FairMQProgOptions::Subscribe<T>(key,Lambda) the types const char* or char* for the calback signatures are not supported.");
        fEvents.Subscribe<fair::mq::PropertyChange, T>(subscriber, func);
    }

    template<typename T>
    void Unsubscribe(const std::string& subscriber) const
    {
        std::lock_guard<std::mutex> lock(fMtx);
        fEvents.Unsubscribe<fair::mq::PropertyChange, T>(subscriber);
    }

    void SubscribeAsString(const std::string& subscriber, std::function<void(typename fair::mq::PropertyChange::KeyType, std::string)> func) const
    {
        std::lock_guard<std::mutex> lock(fMtx);
        fEvents.Subscribe<fair::mq::PropertyChangeAsString, std::string>(subscriber, func);
    }

    void UnsubscribeAsString(const std::string& subscriber) const
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

    void AddChannel(const std::string& name, const FairMQChannel& channel);

    template<typename T>
    static void AddType(std::string label = "")
    {
        if (label == "") {
            label = boost::core::demangle(typeid(T).name());
        }
        fTypeInfos[std::type_index(typeid(T))] = [label](const fair::mq::Property& p) {
            std::stringstream ss;
            ss << boost::any_cast<T>(p);
            return std::pair<std::string, std::string>{ss.str(), label};
        };
    }

    static std::unordered_map<std::type_index, std::function<std::pair<std::string, std::string>(const fair::mq::Property&)>> fTypeInfos;
    static std::unordered_map<std::type_index, void(*)(const fair::mq::EventManager&, const std::string&, const fair::mq::Property&)> fEventEmitters;

  private:
    boost::program_options::variables_map fVarMap; ///< options container

    boost::program_options::options_description fAllOptions; ///< all options descriptions
    boost::program_options::options_description fGeneralOptions; ///< general options descriptions
    boost::program_options::options_description fMQOptions; ///< MQ options descriptions
    boost::program_options::options_description fParserOptions; ///< MQ Parser options descriptions

    mutable std::mutex fMtx;

    std::vector<std::string> fUnregisteredOptions; ///< container with unregistered options

    mutable fair::mq::EventManager fEvents;

    void ParseCmdLine(const int argc, char const* const* argv, bool allowUnregistered = true);
    void ParseDefaults();

    std::unordered_map<std::string, int> GetChannelInfoImpl() const;

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

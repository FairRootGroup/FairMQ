/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PROGOPTIONS_H
#define FAIR_MQ_PROGOPTIONS_H

#include "FairMQChannel.h"
#include "FairMQLogger.h"
#include <fairmq/EventManager.h>
#include <fairmq/ProgOptionsFwd.h>
#include <fairmq/Properties.h>
#include <fairmq/Tools.h>

#include <boost/program_options.hpp>

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace fair
{
namespace mq
{

class ProgOptions
{
  public:
    ProgOptions();
    virtual ~ProgOptions() {}

    void ParseAll(const std::vector<std::string>& cmdArgs, bool allowUnregistered);
    void ParseAll(const int argc, char const* const* argv, bool allowUnregistered = true);
    void Notify();

    void AddToCmdLineOptions(const boost::program_options::options_description optDesc, bool visible = true);
    boost::program_options::options_description& GetCmdLineOptions();

    int Count(const std::string& key) const;

    std::unordered_map<std::string, int> GetChannelInfo() const;
    std::vector<std::string> GetPropertyKeys() const;

    template<typename T>
    T GetProperty(const std::string& key) const
    {
        std::lock_guard<std::mutex> lock(fMtx);
        if (fVarMap.count(key)) {
            return fVarMap[key].as<T>();
        } else {
            LOG(warn) << "Config has no key: " << key << ". Returning default constructed object.";
            return T();
        }
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

    std::string GetPropertyAsString(const std::string& key) const;
    std::string GetPropertyAsString(const std::string& key, const std::string& ifNotFound) const;

    fair::mq::Properties GetProperties(const std::string& q) const;
    fair::mq::Properties GetPropertiesStartingWith(const std::string& q) const;
    std::map<std::string, std::string> GetPropertiesAsString(const std::string& q) const;
    std::map<std::string, std::string> GetPropertiesAsStringStartingWith(const std::string& q) const;

    template<typename T>
    void SetProperty(const std::string& key, T val)
    {
        std::unique_lock<std::mutex> lock(fMtx);

        SetVarMapValue<typename std::decay<T>::type>(key, val);

        lock.unlock();

        fEvents.Emit<fair::mq::PropertyChange, typename std::decay<T>::type>(key, val);
        fEvents.Emit<fair::mq::PropertyChangeAsString, std::string>(key, GetPropertyAsString(key));
    }

    template<typename T>
    bool UpdateProperty(const std::string& key, T val)
    {
        std::unique_lock<std::mutex> lock(fMtx);

        if (fVarMap.count(key)) {
            SetVarMapValue<typename std::decay<T>::type>(key, val);

            lock.unlock();

            fEvents.Emit<fair::mq::PropertyChange, typename std::decay<T>::type>(key, val);
            fEvents.Emit<fair::mq::PropertyChangeAsString, std::string>(key, GetPropertyAsString(key));
            return true;
        } else {
            LOG(debug) << "UpdateProperty failed, no property found with key '" << key << "'";
            return false;
        }
    }

    void SetProperties(const fair::mq::Properties& input);
    bool UpdateProperties(const fair::mq::Properties& input);
    void DeleteProperty(const std::string& key);

    void AddChannel(const std::string& name, const FairMQChannel& channel);

    template<typename T>
    void Subscribe(const std::string& subscriber, std::function<void(typename fair::mq::PropertyChange::KeyType, T)> func) const
    {
        std::lock_guard<std::mutex> lock(fMtx);
        static_assert(!std::is_same<T,const char*>::value || !std::is_same<T, char*>::value,
            "In template member ProgOptions::Subscribe<T>(key,Lambda) the types const char* or char* for the calback signatures are not supported.");
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

    void PrintHelp();
    void PrintOptions();
    void PrintOptionsRaw();

    const boost::program_options::variables_map& GetVarMap() const { return fVarMap; }

    template<typename T>
    T GetValue(const std::string& key) const /* TODO: deprecate this */ { return GetProperty<T>(key); }
    template<typename T>
    int SetValue(const std::string& key, T val) /* TODO: deprecate this */ { SetProperty(key, val); return 0; }
    std::string GetStringValue(const std::string& key) const /* TODO: deprecate this */ { return GetPropertyAsString(key); }

  private:
    void ParseDefaults();
    std::unordered_map<std::string, int> GetChannelInfoImpl() const;

    template<typename T>
    void SetVarMapValue(const std::string& key, const T& val)
    {
        std::map<std::string, boost::program_options::variable_value>& vm = fVarMap;
        vm[key].value() = boost::any(val);
    }

    boost::program_options::variables_map fVarMap; ///< options container
    boost::program_options::options_description fAllOptions; ///< all options descriptions
    std::vector<std::string> fUnregisteredOptions; ///< container with unregistered options

    mutable fair::mq::EventManager fEvents;
    mutable std::mutex fMtx;
};

} // namespace mq
} // namespace fair

#endif /* FAIR_MQ_PROGOPTIONS_H */

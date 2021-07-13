/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PROGOPTIONS_H
#define FAIR_MQ_PROGOPTIONS_H

#include <boost/program_options.hpp>
#include <fairlogger/Logger.h>
#include <fairmq/Channel.h>
#include <fairmq/EventManager.h>
#include <fairmq/ProgOptionsFwd.h>
#include <fairmq/Properties.h>
#include <fairmq/tools/Strings.h>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace fair::mq
{

struct PropertyNotFoundError : std::runtime_error { using std::runtime_error::runtime_error; };

class ProgOptions
{
  public:
    ProgOptions();
    virtual ~ProgOptions() = default;

    void ParseAll(const std::vector<std::string>& cmdArgs, bool allowUnregistered);
    void ParseAll(int argc, char const* const* argv, bool allowUnregistered = true);
    void Notify();

    void AddToCmdLineOptions(boost::program_options::options_description optDesc, bool visible = true);
    boost::program_options::options_description& GetCmdLineOptions();

    /// @brief Checks a property with the given key exist in the configuration
    /// @param key
    /// @return 1 if it exists, 0 otherwise
    int Count(const std::string& key) const;

    /// @brief Retrieve current channel information
    /// @return a map of <channel name, number of subchannels>
    std::unordered_map<std::string, int> GetChannelInfo() const;
    /// @brief Discover the list of property keys
    /// @return list of property keys
    std::vector<std::string> GetPropertyKeys() const;

    /// @brief Read config property, throw if no property with this key exists
    /// @param key
    /// @return config property
    template<typename T>
    T GetProperty(const std::string& key) const
    {
        std::lock_guard<std::mutex> lock(fMtx);
        if (fVarMap.count(key)) {
            return fVarMap[key].as<T>();
        } else {
            throw PropertyNotFoundError(fair::mq::tools::ToString("Config has no key: ", key));
        }
    }

    /// @brief Read config property, return provided value if no property with this key exists
    /// @param key
    /// @param ifNotFound value to return if key is not found
    /// @return config property
    template<typename T>
    T GetProperty(const std::string& key, const T& ifNotFound) const
    {
        std::lock_guard<std::mutex> lock(fMtx);
        if (fVarMap.count(key)) {
            return fVarMap[key].as<T>();
        }
        return ifNotFound;
    }

    /// @brief Read config property as string, throw if no property with this key exists
    /// @param key
    /// @return config property converted to string
    ///
    /// Supports conversion to string for a fixed set of types,
    /// for custom/unsupported types add them via `fair::mq::PropertyHelper::AddType<MyType>("optional label")`
    /// the provided type must then be convertible to string via operator<<
    std::string GetPropertyAsString(const std::string& key) const;
    /// @brief Read config property, return provided value if no property with this key exists
    /// @param key
    /// @param ifNotFound value to return if key is not found
    /// @return config property converted to string
    ///
    /// Supports conversion to string for a fixed set of types,
    /// for custom/unsupported types add them via `fair::mq::PropertyHelper::AddType<MyType>("optional label")`
    /// the provided type must then be convertible to string via operator<<
    std::string GetPropertyAsString(const std::string& key, const std::string& ifNotFound) const;

    /// @brief Read several config properties whose keys match the provided regular expression
    /// @param q regex string to match for
    /// @return container with properties (fair::mq::Properties as an alias for std::map<std::string, Property>, where property is boost::any)
    fair::mq::Properties GetProperties(const std::string& q) const;
    /// @brief Read several config properties whose keys start with the provided string
    /// @param q string to match for
    /// @return container with properties (fair::mq::Properties as an alias for std::map<std::string, Property>, where property is boost::any)
    ///
    /// Typically more performant than GetProperties with regex
    fair::mq::Properties GetPropertiesStartingWith(const std::string& q) const;
    /// @brief Read several config properties as string whose keys match the provided regular expression
    /// @param q regex string to match for
    /// @return container with properties (fair::mq::Properties as an alias for std::map<std::string, Property>, where property is boost::any)
    std::map<std::string, std::string> GetPropertiesAsString(const std::string& q) const;
    /// @brief Read several config properties as string whose keys start with the provided string
    /// @param q string to match for
    /// @return container with properties (fair::mq::Properties as an alias for std::map<std::string, Property>, where property is boost::any)
    ///
    /// Typically more performant than GetPropertiesAsString with regex
    std::map<std::string, std::string> GetPropertiesAsStringStartingWith(const std::string& q) const;

    /// @brief Set config property
    /// @param key
    /// @param val
    template<typename T>
    void SetProperty(const std::string& key, T val)
    {
        std::unique_lock<std::mutex> lock(fMtx);

        SetVarMapValue<typename std::decay<T>::type>(key, val);

        lock.unlock();

        fEvents.Emit<fair::mq::PropertyChange, typename std::decay<T>::type>(key, val);
        fEvents.Emit<fair::mq::PropertyChangeAsString, std::string>(key, GetPropertyAsString(key));
    }

    /// @brief Updates an existing config property (or fails if it doesn't exist)
    /// @param key
    /// @param val
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

    /// @brief Set multiple config properties
    /// @param props fair::mq::Properties as an alias for std::map<std::string, Property>, where property is boost::any
    void SetProperties(const fair::mq::Properties& input);
    /// @brief Updates multiple existing config properties (or fails of any of then do not exist, leaving property store unchanged)
    /// @param props (fair::mq::Properties as an alias for std::map<std::string, Property>, where property is boost::any)
    bool UpdateProperties(const fair::mq::Properties& input);
    /// @brief Deletes a property with the given key from the config store
    /// @param key
    void DeleteProperty(const std::string& key);

    /// @brief Takes the provided channel and creates properties based on it
    /// @param name channel name
    /// @param channel FairMQChannel reference
    void AddChannel(const std::string& name, const Channel& channel);

    /// @brief Subscribe to property updates of type T
    /// @param subscriber
    /// @param callback function
    ///
    /// Subscribe to property changes with a callback to monitor property changes in an event based fashion.
    template<typename T>
    void Subscribe(const std::string& subscriber, std::function<void(typename fair::mq::PropertyChange::KeyType, T)> func) const
    {
        std::lock_guard<std::mutex> lock(fMtx);
        static_assert(!std::is_same<T,const char*>::value || !std::is_same<T, char*>::value,
            "In template member ProgOptions::Subscribe<T>(key,Lambda) the types const char* or char* for the calback signatures are not supported.");
        fEvents.Subscribe<fair::mq::PropertyChange, T>(subscriber, func);
    }

    /// @brief Unsubscribe from property updates of type T
    /// @param subscriber
    template<typename T>
    void Unsubscribe(const std::string& subscriber) const
    {
        std::lock_guard<std::mutex> lock(fMtx);
        fEvents.Unsubscribe<fair::mq::PropertyChange, T>(subscriber);
    }

    /// @brief Subscribe to property updates, with values converted to string
    /// @param subscriber
    /// @param callback function
    ///
    /// Subscribe to property changes with a callback to monitor property changes in an event based fashion. Will convert the property to string.
    void SubscribeAsString(const std::string& subscriber, std::function<void(typename fair::mq::PropertyChange::KeyType, std::string)> func) const
    {
        std::lock_guard<std::mutex> lock(fMtx);
        fEvents.Subscribe<fair::mq::PropertyChangeAsString, std::string>(subscriber, func);
    }

    /// @brief Unsubscribe from property updates that convert to string
    /// @param subscriber
    void UnsubscribeAsString(const std::string& subscriber) const
    {
        std::lock_guard<std::mutex> lock(fMtx);
        fEvents.Unsubscribe<fair::mq::PropertyChangeAsString, std::string>(subscriber);
    }

    /// @brief prints full options description
    void PrintHelp() const;
    /// @brief prints properties stored in the property container
    void PrintOptions() const;
    /// @brief prints full options description in a compact machine-readable format
    void PrintOptionsRaw() const;

    /// @brief returns the property container
    const boost::program_options::variables_map& GetVarMap() const { return fVarMap; }

    /// @brief Read config property, return default-constructed object if key doesn't exist
    /// @param key
    /// @return config property
    template<typename T>
    T GetValue(const std::string& key) const /* TODO: deprecate this */
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
    int SetValue(const std::string& key, T val) /* TODO: deprecate this */ { SetProperty(key, val); return 0; }
    /// @brief Read config property as string, return default-constructed object if key doesn't exist
    /// @param key
    /// @return config property
    std::string GetStringValue(const std::string& key) const; /* TODO: deprecate this */

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

} // namespace fair::mq

#endif /* FAIR_MQ_PROGOPTIONS_H */

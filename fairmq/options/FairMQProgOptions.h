/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
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

#include <fairmq/EventManager.h>

#include "FairProgOptions.h"
#include "FairMQChannel.h"

#include <unordered_map>
#include <functional>
#include <map>
#include <mutex>
#include <string>

namespace fair
{
namespace mq
{

struct PropertyChange : Event<std::string> {};
struct PropertyChangeAsString : Event<std::string> {};

} /* namespace mq */
} /* namespace fair */

class FairMQProgOptions : public FairProgOptions
{
  protected:
    using FairMQMap = std::unordered_map<std::string, std::vector<FairMQChannel>>;

  public:
    FairMQProgOptions();
    virtual ~FairMQProgOptions();

    int ParseAll(const std::vector<std::string>& cmdLineArgs, bool allowUnregistered);
    // parse command line.
    // default parser for the mq-configuration file (JSON/XML) is called if command line key mq-config is called
    int ParseAll(const int argc, char const* const* argv, bool allowUnregistered = false) override;

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
            LOG(error) << e.what();
            return 1;
        }
        return 0;
    }

    FairMQMap GetFairMQMap() const
    {
        return fFairMQMap;
    }

    std::unordered_map<std::string, int> GetChannelInfo() const
    {
        return fChannelInfo;
    }

    // store key-value of type T into variable_map.
    // If key is found in fMQKeyMap, update the FairMQChannelMap accordingly
    // Note that the fMQKeyMap is filled:
    // - if UpdateChannelMap(const FairMQMap& map) method is called
    // - if UserParser template method is called (it is called in the ParseAll method if json or xml MQ-config files is provided)

    // specialization/overloading for string, pass by const ref
    int UpdateValue(const std::string& key, const std::string& val) // string API
    {
        UpdateValue(key, val);
        return 0;
    }

    int UpdateValue(const std::string& key, const char* val) // string API
    {
        UpdateValue<std::string>(key, std::string(val));
        return 0;
    }

    template<typename T>
    int UpdateValue(const std::string& key, T val)
    {
        std::unique_lock<std::mutex> lock(fConfigMutex);

        if (fVarMap.count(key))
        {
            // update variable map
            UpdateVarMap<typename std::decay<T>::type>(key, val);

            // update FairMQChannel map, check first if data are int or string
            if (std::is_same<T, int>::value || std::is_same<T, std::string>::value)
            {
                if (fMQKeyMap.count(key))
                {
                    std::string channelName;
                    int index = 0;
                    std::string member;
                    std::tie(channelName, index, member) = fMQKeyMap.at(key);
                    UpdateChannelMap(channelName, index, member, val);
                }
            }

            lock.unlock();
            //if (std::is_same<T, int>::value || std::is_same<T, std::string>::value)//if one wants to restrict type
            fEvents.Emit<fair::mq::PropertyChange, typename std::decay<T>::type>(key, val);
            fEvents.Emit<fair::mq::PropertyChangeAsString, std::string>(key, GetStringValue(key));

            return 0;
        }
        else
        {
            LOG(error) << "UpdateValue failed: key '" << key << "' not found in the variable map";
            return 1;
        }
        return 0;
    }

    template<typename T>
    int SetValue(const std::string& key, T val)
    {
        std::unique_lock<std::mutex> lock(fConfigMutex);

        // update variable map
        UpdateVarMap<typename std::decay<T>::type>(key, val);

        // update FairMQChannel map, check first if data are int or string
        if (std::is_same<T, int>::value || std::is_same<T, std::string>::value)
        {
            if (fMQKeyMap.count(key))
            {
                std::string channelName;
                int index = 0;
                std::string member;
                std::tie(channelName, index, member) = fMQKeyMap.at(key);
                UpdateChannelMap(channelName, index, member, val);
            }
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

    // replace FairMQChannelMap, and update variable map accordingly
    int UpdateChannelMap(const FairMQMap& map);

  protected:
    po::options_description fMQCmdOptions;
    po::options_description fMQParserOptions;
    FairMQMap fFairMQMap;

    // map of read channel info - channel name - number of subchannels
    std::unordered_map<std::string, int> fChannelInfo;

    using MQKey = std::tuple<std::string, int, std::string>;//store key info
    std::map<std::string, MQKey> fMQKeyMap;// key=full path - val=key info

    int ImmediateOptions() override; // for custom help & version printing
    void InitOptionDescription();

    // read FairMQChannelMap and insert/update corresponding values in variable map
    // create key for variable map as follow : channelName.index.memberName
    void UpdateMQValues();
    int Store(const FairMQMap& channels);

  private:
    template<typename T>
    void EmitUpdate(const std::string& key, T val)
    {
        //compile time check whether T is const char* or char*, and in that case a compile time error is thrown.
        static_assert(!std::is_same<T,const char*>::value || !std::is_same<T, char*>::value,
            "In template member FairMQProgOptions::EmitUpdate<T>(key,val) the types const char* or char* for the calback signatures are not supported.");
        fEvents.Emit<fair::mq::PropertyChange, T>(key, val);
        fEvents.Emit<fair::mq::PropertyChangeAsString, std::string>(key, GetStringValue(key));
    }

    int UpdateChannelMap(const std::string& channelName, int index, const std::string& member, const std::string& val);
    int UpdateChannelMap(const std::string& channelName, int index, const std::string& member, int val);
    // for cases other than int and string
    template<typename T>
    int UpdateChannelMap(const std::string& /*channelName*/, int /*index*/, const std::string& /*member*/, T /*val*/)
    {
        return 0;
    }

    void UpdateChannelInfo();

    fair::mq::EventManager fEvents;
};

#endif /* FAIRMQPROGOPTIONS_H */

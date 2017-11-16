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

#include <fairmq/EventManager.h>

#include <unordered_map>
#include <functional>
#include <map>
#include <set>
#include <mutex>
#include <string>

#include "FairProgOptions.h"
#include "FairMQChannel.h"

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
    //using signal_type = boost::signals2::signal<void(const std::string&, const std::string&)>;// string API
    //using signal_type_ptr = boost::shared_ptr<signal_type>;// string API

  public:
    FairMQProgOptions();
    virtual ~FairMQProgOptions();

    void ParseAll(const std::vector<std::string>& cmdLineArgs, bool allowUnregistered);
    // parse command line and txt/INI configuration file.
    // default parser for the mq-configuration file (JSON/XML) is called if command line key mq-config is called
    virtual void ParseAll(const int argc, char const* const* argv, bool allowUnregistered = false);

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

    FairMQMap GetFairMQMap() const
    {
        return fFairMQMap;
    }

    std::unordered_map<std::string, int> GetChannelInfo() const
    {
        return fChannelInfo;
    }

    // to customize title of the executable help command line  
    void SetHelpTitle(const std::string& title)
    {
        fHelpTitle = title;
    }
    // to customize the executable version command line
    void SetVersion(const std::string& version)
    {
        fVersion = version;
    }

    // store key-value of type T into variable_map. 
    // If key is found in fMQKeyMap, update the FairMQChannelMap accordingly
    // Note that the fMQKeyMap is filled:
    // - if UpdateChannelMap(const FairMQMap& map) method is called 
    // - if UserParser template method is called (it is called in the ParseAll method if json or xml MQ-config files is provided)
    

    /* // string API

    //overload for string literal
    int UpdateValue(const std::string& key, const char* val) // string API
    {
        UpdateValue(key,std::string(val));
        return 0;
    }
    // overload for string values
    int UpdateValue(const std::string& key, const std::string& val) // string API
    {
        try
        {
            if (fVarMap.count(key))
            {


                if (!FairMQ::is_this_type<std::string>(fVarMap.at(key)))
                {
                    LOG(ERROR) << "You try to update a value as string (for key="<< key <<") while it has been defined with a different type in the option description.";
                    abort();
                }

                // update variable map
                UpdateVarMap(key,val);

                if (fMQKeyMap.count(key))
                {
                    std::string channelName;
                    int index = 0;
                    std::string member;
                    std::tie(channelName, index, member) = fMQKeyMap.at(key);
                    UpdateChannelMap(channelName, index, member, val);
                }

                // execute stored function of a given key if exist
                //if (std::is_same<T, int>::value || std::is_same<T, std::string>::value)//if one wants to restrict type
                if (fSignalMap.count(key))
                    EmitUpdate(key,val);

                return 0;
            }
            else
            {

                LOG(ERROR)  <<"UpdatedValue failed because the provided key '"
                            <<key
                            <<"' is not found in the variable map";
                return 1;
            }
        }
        catch (std::exception& e)
        {
            LOG(ERROR)  << "Caught exception on key "<<key; 
            abort();
        }
        
        return 0;
    }
    */


    //overload for string literal
    /*int UpdateValue(const std::string& key, const char* val) // string API
    {
        UpdateValue<std::string>(key,val);
        return 0;
    }*/


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
            LOG(ERROR) << "UpdatedValue failed because the provided key '"
                       << key
                       << "' is not found in the variable map";
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
    /*
    template <typename F>
    void Subscribe(const std::string& key, F&& func) 
    {
        if (fVarMap.count(key))
        {
            //if key-value not yet found, then add it
            if (fSignalMap.find(key) == fSignalMap.end())
                fSignalMap.emplace(key, boost::make_shared<signal_type>());
            (*fSignalMap.at(key)).connect(std::forward<F>(func));
        }
    }
    */

    // replace FairMQChannelMap, and update variable map accordingly
    int UpdateChannelMap(const FairMQMap& map);

  protected:
    po::options_description fMQParserOptions;
    po::options_description fMQOptionsInCfg;
    po::options_description fMQOptionsInCmd;
    FairMQMap fFairMQMap;
    std::string fHelpTitle;
    std::string fVersion;

    // map of read channel info - channel name - number of subchannels
    std::unordered_map<std::string, int> fChannelInfo;

    using MQKey = std::tuple<std::string, int, std::string>;//store key info
    std::map<std::string, MQKey> fMQKeyMap;// key=full path - val=key info

    virtual int NotifySwitchOption(); // for custom help & version printing
    void InitOptionDescription();

    // fill boost option description with the standard options
    static void FillOptionDescription(po::options_description& options);

    // read FairMQChannelMap and insert/update corresponding values in variable map
    // create key for variable map as follow : channelName.index.memberName
    void UpdateMQValues();
    int Store(const FairMQMap& channels);

  private:
    /*
    // string API
    std::map<std::string, signal_type_ptr > fSignalMap;
    void EmitUpdate(const std::string& key, const char* val)
    {
        EmitUpdate(key,std::string(val));
    }
    void EmitUpdate(const std::string& key, const std::string& val)
    {
        (*fSignalMap.at(key))(key,val);
    }
    */

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

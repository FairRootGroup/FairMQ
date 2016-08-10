/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

/*
 * File:   FairMQEventManager.h
 * Author: winckler
 *
 * Created on August 12, 2016, 13:50 PM
 */

#ifndef FAIRMQEVENTMANAGER_H
#define FAIRMQEVENTMANAGER_H

#include <map>
#include <utility>
#include <string>

#include <boost/any.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>
#include <boost/signals2/signal.hpp>

enum class EventId : uint32_t
{
    // Place your new EventManager events here
    UpdateParam           = 0,
    Custom                = 1,
};

namespace Events
{

template <EventId,typename ...Args> struct Traits;
template <typename T> struct Traits<EventId::UpdateParam, T>       { using signal_type = boost::signals2::signal<void(const std::string&, T)>; } ;
template <typename T> struct Traits<EventId::UpdateParam, std::vector<T> >       { using signal_type = boost::signals2::signal<void(const std::string&, const std::vector<T>& )>; } ;

template <> struct Traits<EventId::UpdateParam, std::string>       { using signal_type = boost::signals2::signal<void(const std::string&, const std::string&)>; } ;

template<std::size_t N> struct Traits<EventId::UpdateParam, const char[N]>       { using signal_type = boost::signals2::signal<void(const std::string&, const std::string&)>; } ;

template <typename ...T> struct Traits<EventId::Custom,T...> { using signal_type = boost::signals2::signal<void(T...)>; } ;

/*
template <EventId, typename ...Args> struct Traits2;
template <> struct Traits2<EventId::UpdateParam> { using signal_type = boost::signals2::signal<void(const std::string&, const std::string&)>; } ;
template <typename ...T> struct Traits2<EventId::UpdateParam,T...> { using signal_type = boost::signals2::signal<void(const std::string&, T...)>; } ;
template <> struct Traits2<EventId::UpdateParamInt>    { using signal_type = boost::signals2::signal<void(const std::string&, int)>; } ;
// */

} // Events namespace

class FairMQEventManager
{
  public:
    typedef std::pair<EventId,std::string> EventKey;
    
    FairMQEventManager() : fEventMap() {}
    virtual ~FairMQEventManager(){}
    

    template <EventId event, typename... ValueType, typename F>
    void Connect(const std::string& key, F&& func) 
    {
        GetSlot<event,ValueType...>(key).connect(std::forward<F>(func));
    }
    
    template <EventId event, typename... ValueType>
    void Disonnect(const std::string& key) 
    {
        GetSlot<event,ValueType...>(key).disconnect();
    }


    template <EventId event, typename... ValueType, typename... Args>
    void Emit(const std::string& key, Args&&... args) 
    {
        GetSlot<event,ValueType...>(key)(std::forward<Args>(args)...);
    }


    template <EventId event>
    bool EventKeyFound(const std::string& key)
    {
        if (fEventMap.find(std::pair<EventId,std::string>(event,key) ) != fEventMap.end())
            return true;
        else
            return false;
    }

  private:
    std::map<EventKey, boost::any> fEventMap;

    template <EventId event, typename... T, typename Slot = typename Events::Traits<event,T...>::signal_type, 
    typename SlotPtr = boost::shared_ptr<Slot> >
    Slot& GetSlot(const std::string& key)
    {
        try
        {
            EventKey eventKey = std::make_pair(event,key);
            //static_assert(std::is_same<decltype(boost::make_shared<Slot>()),SlotPtr>::value, "");
            if (fEventMap.find(eventKey) == fEventMap.end())
                fEventMap.emplace(eventKey, boost::make_shared<Slot>());

            return *boost::any_cast<SlotPtr>(fEventMap.at(eventKey));
//            auto &&tmp = boost::any_cast<SlotPtr>(fEventMap.at(eventKey));
//            return *tmp;
        }
        catch (boost::bad_any_cast const &e)
        {
            LOG(ERROR)  << "Caught instance of boost::bad_any_cast: " 
                        << e.what() << " on event #" << static_cast<uint32_t>(event) << " and key" << key;
            abort();
        }
    }
};

#endif /* FAIRMQEVENTMANAGER_H */

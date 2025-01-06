/********************************************************************************
 * Copyright (C) 2014-2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_EVENTMANAGER_H
#define FAIR_MQ_EVENTMANAGER_H

#include <boost/any.hpp>
#include <boost/functional/hash.hpp>
#include <boost/signals2.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace fair::mq {

// Inherit from this base event type to create custom event types
template<typename K>
struct Event
{
    using KeyType = K;
};

/**
 * @class EventManager EventManager.h <fairmq/EventManager.h>
 * @brief Manages event callbacks from different subscribers
 *
 * The event manager stores a set of callbacks and associates them with
 * events depending on the callback signature. The first callback
 * argument must be of a special key type determined by the event type.
 *
 * Callbacks can be subscribed/unsubscribed based on a subscriber id,
 * the event type, and the callback signature.
 *
 * Events can be emitted based on event type and callback signature.
 *
 * The event manager is thread-safe.
 */
class EventManager
{
  public:
    // Clang 3.4-3.8 has a bug and cannot properly deal with the following template alias.
    // Therefore, we leave them here commented out for now.
    // template<typename E, typename ...Args>
    // using Callback = std::function<void(typename E::KeyType, Args...)>;

    template<typename E, typename... Args>
    using Signal = boost::signals2::signal<void(typename E::KeyType, Args...)>;

    template<typename E, typename... Args>
    auto Subscribe(const std::string& subscriber,
                   std::function<void(typename E::KeyType, Args...)> callback) -> void;

    template<typename E, typename... Args>
    auto Unsubscribe(const std::string& subscriber) -> void
    {
        const std::type_index event_type_index{typeid(E)};
        const std::type_index callback_type_index{
            typeid(std::function<void(typename E::KeyType, Args...)>)};
        const auto signalsKey = std::make_pair(event_type_index, callback_type_index);
        const auto connectionsKey = std::make_pair(subscriber, signalsKey);

        std::lock_guard<std::mutex> lock{fMutex};

        fConnections.at(connectionsKey).disconnect();
        fConnections.erase(connectionsKey);
    }

    template<typename E, typename... Args>
    auto Emit(typename E::KeyType key, Args... args) const -> void
    {
        const std::type_index event_type_index{typeid(E)};
        const std::type_index callback_type_index{
            typeid(std::function<void(typename E::KeyType, Args...)>)};
        const auto signalsKey = std::make_pair(event_type_index, callback_type_index);

        (*GetSignal<E, Args...>(signalsKey))(key, std::forward<Args>(args)...);
    }

  private:
    using SignalsKey = std::pair<std::type_index, std::type_index>;
    // event          , callback
    using SignalsValue = boost::any;
    using SignalsMap = std::unordered_map<SignalsKey, SignalsValue, boost::hash<SignalsKey>>;
    mutable SignalsMap fSignals;

    using ConnectionsKey = std::pair<std::string, SignalsKey>;
    // subscriber , event/callback
    using ConnectionsValue = boost::signals2::connection;
    using ConnectionsMap =
        std::unordered_map<ConnectionsKey, ConnectionsValue, boost::hash<ConnectionsKey>>;
    ConnectionsMap fConnections;

    mutable std::mutex fMutex;

    template<typename E, typename... Args>
    auto GetSignal(const SignalsKey& key) const -> std::shared_ptr<Signal<E, Args...>>;
}; /* class EventManager */

struct PropertyChangeAsString : Event<std::string>
{};

template<typename E, typename... Args>
auto EventManager::GetSignal(const SignalsKey& key) const -> std::shared_ptr<Signal<E, Args...>>
{
    std::lock_guard<std::mutex> lock{fMutex};

    if (fSignals.find(key) == fSignals.end()) {
        // wrapper is needed because boost::signals2::signal is neither copyable nor movable
        // and I don't know how else to insert it into the map
        auto signal = std::make_shared<Signal<E, Args...>>();
        fSignals.insert(std::make_pair(key, signal));
    }

    return boost::any_cast<std::shared_ptr<Signal<E, Args...>>>(fSignals.at(key));
}

template<typename E, typename... Args>
auto EventManager::Subscribe(const std::string& subscriber,
                             std::function<void(typename E::KeyType, Args...)> callback) -> void
{
    const std::type_index event_type_index{typeid(E)};
    const std::type_index callback_type_index{
        typeid(std::function<void(typename E::KeyType, Args...)>)};
    const auto signalsKey = std::make_pair(event_type_index, callback_type_index);
    const auto connectionsKey = std::make_pair(subscriber, signalsKey);

    const auto connection = GetSignal<E, Args...>(signalsKey)->connect(callback);

    {
        std::lock_guard<std::mutex> lock{fMutex};

        if (fConnections.find(connectionsKey) != fConnections.end()) {
            fConnections.at(connectionsKey).disconnect();
            fConnections.erase(connectionsKey);
        }
        fConnections.insert({connectionsKey, connection});
    }
}

extern template std::shared_ptr<
    fair::mq::EventManager::Signal<fair::mq::PropertyChangeAsString, std::string>>
    fair::mq::EventManager::GetSignal<fair::mq::PropertyChangeAsString, std::string>(
        const std::pair<std::type_index, std::type_index>& key) const;

extern template void
    fair::mq::EventManager::Subscribe<fair::mq::PropertyChangeAsString, std::string>(
        const std::string& subscriber,
        std::function<void(typename fair::mq::PropertyChangeAsString::KeyType, std::string)>);

}   // namespace fair::mq

#endif /* FAIR_MQ_EVENTMANAGER_H */

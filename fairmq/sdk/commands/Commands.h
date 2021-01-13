/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_COMMANDFACTORY
#define FAIR_MQ_SDK_COMMANDFACTORY

#include <fairmq/States.h>

#include <vector>
#include <string>
#include <memory>
#include <type_traits>
#include <stdexcept>

namespace fair::mq::sdk::cmd
{

enum class Format : int {
    Binary,
    JSON
};

enum class Result : int {
    Ok,
    Failure
};

enum class Type : int
{
    check_state,                   // args: { }
    change_state,                  // args: { transition }
    dump_config,                   // args: { }
    subscribe_to_state_change,     // args: { }
    unsubscribe_from_state_change, // args: { }
    state_change_exiting_received, // args: { }
    get_properties,                // args: { request_id, property_query }
    set_properties,                // args: { request_id, properties }
    subscription_heartbeat,        // args: { interval }

    current_state,                 // args: { device_id, current_state }
    transition_status,             // args: { device_id, task_id, Result, transition, current_state }
    config,                        // args: { device_id, config_string }
    state_change_subscription,     // args: { device_id, task_id, Result }
    state_change_unsubscription,   // args: { device_id, task_id, Result }
    state_change,                  // args: { device_id, task_id, last_state, current_state }
    properties,                    // args: { device_id, request_id, Result, properties }
    properties_set                 // args: { device_id, request_id, Result }
};

struct Cmd
{
    explicit Cmd(const Type type) : fType(type) {}
    virtual ~Cmd() = default;

    Type GetType() const { return fType; }

  private:
    Type fType;
};

struct CheckState : Cmd
{
    explicit CheckState() : Cmd(Type::check_state) {}
};

struct ChangeState : Cmd
{
    explicit ChangeState(Transition transition)
        : Cmd(Type::change_state)
        , fTransition(transition)
    {}

    Transition GetTransition() const { return fTransition; }
    void SetTransition(Transition transition) { fTransition = transition; }

  private:
    Transition fTransition;
};

struct DumpConfig : Cmd
{
    explicit DumpConfig() : Cmd(Type::dump_config) {}
};

struct SubscribeToStateChange : Cmd
{
    explicit SubscribeToStateChange(int64_t interval)
        : Cmd(Type::subscribe_to_state_change)
        , fInterval(interval)
    {}

    int64_t GetInterval() const { return fInterval; }
    void SetInterval(int64_t interval) { fInterval = interval; }

  private:
    int64_t fInterval;
};

struct UnsubscribeFromStateChange : Cmd
{
    explicit UnsubscribeFromStateChange() : Cmd(Type::unsubscribe_from_state_change) {}
};

struct StateChangeExitingReceived : Cmd
{
    explicit StateChangeExitingReceived() : Cmd(Type::state_change_exiting_received) {}
};

struct GetProperties : Cmd
{
    GetProperties(std::size_t request_id, std::string query)
        : Cmd(Type::get_properties)
        , fRequestId(request_id)
        , fQuery(std::move(query))
    {}

    auto GetRequestId() const -> std::size_t { return fRequestId; }
    auto SetRequestId(std::size_t requestId) -> void { fRequestId = requestId; }
    auto GetQuery() const -> std::string { return fQuery; }
    auto SetQuery(std::string query) -> void { fQuery = std::move(query); }

  private:
    std::size_t fRequestId;
    std::string fQuery;
};

struct SetProperties : Cmd
{
    SetProperties(std::size_t request_id, std::vector<std::pair<std::string, std::string>> properties)
        : Cmd(Type::set_properties)
        , fRequestId(request_id)
        , fProperties(std::move(properties))
    {}

    auto GetRequestId() const -> std::size_t { return fRequestId; }
    auto SetRequestId(std::size_t requestId) -> void { fRequestId = requestId; }
    auto GetProps() const -> std::vector<std::pair<std::string, std::string>> { return fProperties; }
    auto SetProps(std::vector<std::pair<std::string, std::string>> properties) -> void { fProperties = std::move(properties); }

  private:
    std::size_t fRequestId;
    std::vector<std::pair<std::string, std::string>> fProperties;
};

struct SubscriptionHeartbeat : Cmd
{
    explicit SubscriptionHeartbeat(int64_t interval)
        : Cmd(Type::subscription_heartbeat)
        , fInterval(interval)
    {}

    int64_t GetInterval() const { return fInterval; }
    void SetInterval(int64_t interval) { fInterval = interval; }

  private:
    int64_t fInterval;
};

struct CurrentState : Cmd
{
    explicit CurrentState(const std::string& id, State currentState)
        : Cmd(Type::current_state)
        , fDeviceId(id)
        , fCurrentState(currentState)
    {}

    std::string GetDeviceId() const { return fDeviceId; }
    void SetDeviceId(const std::string& deviceId) { fDeviceId = deviceId; }
    fair::mq::State GetCurrentState() const { return fCurrentState; }
    void SetCurrentState(fair::mq::State state) { fCurrentState = state; }

  private:
    std::string fDeviceId;
    fair::mq::State fCurrentState;
};

struct TransitionStatus : Cmd
{
    explicit TransitionStatus(const std::string& deviceId, const uint64_t taskId, const Result result, const Transition transition, State currentState)
        : Cmd(Type::transition_status)
        , fDeviceId(deviceId)
        , fTaskId(taskId)
        , fResult(result)
        , fTransition(transition)
        , fCurrentState(currentState)
    {}

    std::string GetDeviceId() const { return fDeviceId; }
    void SetDeviceId(const std::string& deviceId) { fDeviceId = deviceId; }
    uint64_t GetTaskId() const { return fTaskId; }
    void SetTaskId(const uint64_t taskId) { fTaskId = taskId; }
    Result GetResult() const { return fResult; }
    void SetResult(const Result result) { fResult = result; }
    Transition GetTransition() const { return fTransition; }
    void SetTransition(const Transition transition) { fTransition = transition; }
    fair::mq::State GetCurrentState() const { return fCurrentState; }
    void SetCurrentState(fair::mq::State state) { fCurrentState = state; }

  private:
    std::string fDeviceId;
    uint64_t fTaskId;
    Result fResult;
    Transition fTransition;
    fair::mq::State fCurrentState;
};

struct Config : Cmd
{
    explicit Config(const std::string& id, const std::string& config)
        : Cmd(Type::config)
        , fDeviceId(id)
        , fConfig(config)
    {}

    std::string GetDeviceId() const { return fDeviceId; }
    void SetDeviceId(const std::string& deviceId) { fDeviceId = deviceId; }
    std::string GetConfig() const { return fConfig; }
    void SetConfig(const std::string& config) { fConfig = config; }

  private:
    std::string fDeviceId;
    std::string fConfig;
};

struct StateChangeSubscription : Cmd
{
    explicit StateChangeSubscription(const std::string& id, const uint64_t taskId, const Result result)
        : Cmd(Type::state_change_subscription)
        , fDeviceId(id)
        , fTaskId(taskId)
        , fResult(result)
    {}

    std::string GetDeviceId() const { return fDeviceId; }
    void SetDeviceId(const std::string& deviceId) { fDeviceId = deviceId; }
    uint64_t GetTaskId() const { return fTaskId; }
    void SetTaskId(const uint64_t taskId) { fTaskId = taskId; }
    Result GetResult() const { return fResult; }
    void SetResult(const Result result) { fResult = result; }

  private:
    std::string fDeviceId;
    uint64_t fTaskId;
    Result fResult;
};

struct StateChangeUnsubscription : Cmd
{
    explicit StateChangeUnsubscription(const std::string& id, const uint64_t taskId, const Result result)
        : Cmd(Type::state_change_unsubscription)
        , fDeviceId(id)
        , fTaskId(taskId)
        , fResult(result)
    {}

    std::string GetDeviceId() const { return fDeviceId; }
    void SetDeviceId(const std::string& deviceId) { fDeviceId = deviceId; }
    uint64_t GetTaskId() const { return fTaskId; }
    void SetTaskId(const uint64_t taskId) { fTaskId = taskId; }
    Result GetResult() const { return fResult; }
    void SetResult(const Result result) { fResult = result; }

  private:
    std::string fDeviceId;
    uint64_t fTaskId;
    Result fResult;
};

struct StateChange : Cmd
{
    explicit StateChange(const std::string& deviceId, const uint64_t taskId, const State lastState, const State currentState)
        : Cmd(Type::state_change)
        , fDeviceId(deviceId)
        , fTaskId(taskId)
        , fLastState(lastState)
        , fCurrentState(currentState)
    {}

    std::string GetDeviceId() const { return fDeviceId; }
    void SetDeviceId(const std::string& deviceId) { fDeviceId = deviceId; }
    uint64_t GetTaskId() const { return fTaskId; }
    void SetTaskId(const uint64_t taskId) { fTaskId = taskId; }
    fair::mq::State GetLastState() const { return fLastState; }
    void SetLastState(const fair::mq::State state) { fLastState = state; }
    fair::mq::State GetCurrentState() const { return fCurrentState; }
    void SetCurrentState(const fair::mq::State state) { fCurrentState = state; }

  private:
    std::string fDeviceId;
    uint64_t fTaskId;
    fair::mq::State fLastState;
    fair::mq::State fCurrentState;
};

struct Properties : Cmd
{
    Properties(std::string deviceId, std::size_t requestId, const Result result, std::vector<std::pair<std::string, std::string>> properties)
        : Cmd(Type::properties)
        , fDeviceId(std::move(deviceId))
        , fRequestId(requestId)
        , fResult(result)
        , fProperties(std::move(properties))
    {}

    auto GetDeviceId() const -> std::string { return fDeviceId; }
    auto SetDeviceId(std::string deviceId) -> void { fDeviceId = std::move(deviceId); }
    auto GetRequestId() const -> std::size_t { return fRequestId; }
    auto SetRequestId(std::size_t requestId) -> void { fRequestId = requestId; }
    auto GetResult() const -> Result { return fResult; }
    auto SetResult(Result result) -> void { fResult = result; }
    auto GetProps() const -> std::vector<std::pair<std::string, std::string>> { return fProperties; }
    auto SetProps(std::vector<std::pair<std::string, std::string>> properties) -> void { fProperties = std::move(properties); }

  private:
    std::string fDeviceId;
    std::size_t fRequestId;
    Result fResult;
    std::vector<std::pair<std::string, std::string>> fProperties;
};

struct PropertiesSet : Cmd {
    PropertiesSet(std::string deviceId, std::size_t requestId, Result result)
        : Cmd(Type::properties_set)
        , fDeviceId(std::move(deviceId))
        , fRequestId(requestId)
        , fResult(result)
    {}

    auto GetDeviceId() const -> std::string { return fDeviceId; }
    auto SetDeviceId(std::string deviceId) -> void { fDeviceId = std::move(deviceId); }
    auto GetRequestId() const -> std::size_t { return fRequestId; }
    auto SetRequestId(std::size_t requestId) -> void { fRequestId = requestId; }
    auto GetResult() const -> Result { return fResult; }
    auto SetResult(Result result) -> void { fResult = result; }

  private:
    std::string fDeviceId;
    std::size_t fRequestId;
    Result fResult;
};

template<typename C, typename... Args>
std::unique_ptr<Cmd> make(Args&&... args)
{
    return std::make_unique<C>(std::forward<Args>(args)...);
}

struct Cmds
{
    using container = std::vector<std::unique_ptr<Cmd>>;
    struct CommandFormatError : std::runtime_error { using std::runtime_error::runtime_error; };

    explicit Cmds() {}

    template<typename... Rest>
    explicit Cmds(std::unique_ptr<Cmd>&& first, Rest&&... rest)
    {
        Unpack(std::forward<std::unique_ptr<Cmd>&&>(first), std::forward<Rest>(rest)...);
    }

    void Add(std::unique_ptr<Cmd>&& cmd) { fCmds.emplace_back(std::move(cmd)); }

    template<typename C, typename... Args>
    void Add(Args&&... args)
    {
        static_assert(std::is_base_of<Cmd, C>::value, "Only types derived from fair::mq::cmd::Cmd are allowed");
        Add(make<C>(std::forward<Args>(args)...));
    }

    Cmd& At(size_t i) { return *(fCmds.at(i)); }

    size_t Size() const { return fCmds.size(); }
    void Reset() { fCmds.clear(); }

    std::string Serialize(const Format type = Format::Binary) const;
    void Deserialize(const std::string&, const Format type = Format::Binary);

  private:
    container fCmds;

    void Unpack() {}

    template <class... Rest>
    void Unpack(std::unique_ptr<Cmd>&& first, Rest&&... rest)
    {
        fCmds.emplace_back(std::move(first));
        Unpack(std::forward<Rest>(rest)...);
    }

  public:
    using iterator = container::iterator;
    using const_iterator = container::const_iterator;

    auto begin() -> decltype(fCmds.begin()) { return fCmds.begin(); }
    auto end() -> decltype(fCmds.end()) { return fCmds.end(); }
    auto cbegin() -> decltype(fCmds.cbegin()) { return fCmds.cbegin(); }
    auto cend() -> decltype(fCmds.cend()) { return fCmds.cend(); }
};

std::string GetResultName(const Result result);
std::string GetTypeName(const Type type);

inline std::ostream& operator<<(std::ostream& os, const Result& result) { return os << GetResultName(result); }
inline std::ostream& operator<<(std::ostream& os, const Type& type) { return os << GetTypeName(type); }

} // namespace fair::mq::sdk::cmd

#endif /* FAIR_MQ_SDK_COMMANDFACTORY */

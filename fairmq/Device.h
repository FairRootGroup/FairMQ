/********************************************************************************
 *   Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH     *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_DEVICE_H
#define FAIR_MQ_DEVICE_H

#include <algorithm>   // find
#include <atomic>
#include <chrono>
#include <cstddef>
#include <fairlogger/Logger.h>
#include <fairmq/Channel.h>
#include <fairmq/Message.h>
#include <fairmq/Parts.h>
#include <fairmq/ProgOptions.h>
#include <fairmq/StateMachine.h>
#include <fairmq/StateQueue.h>
#include <fairmq/Tools.h>
#include <fairmq/TransportFactory.h>
#include <fairmq/Transports.h>
#include <fairmq/UnmanagedRegion.h>
#include <functional>
#include <memory>   // unique_ptr
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>   // pair
#include <vector>

namespace fair::mq {

using ChannelMap = std::unordered_map<std::string, std::vector<Channel>>;

struct OngoingTransition : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

using InputMsgCallback = std::function<bool(MessagePtr&, int)>;

using InputMultipartCallback = std::function<bool(Parts&, int)>;

class Device
{
    friend class Channel;

  public:
    Device();
    Device(ProgOptions& config);
    Device(tools::Version version);
    Device(ProgOptions& config, tools::Version version);

  private:
    Device(ProgOptions* config, tools::Version version);

  public:
    Device(const Device&) = delete;
    Device operator=(const Device&) = delete;
    virtual ~Device();

    /// Outputs the socket transfer rates
    virtual void LogSocketRates();

    template<typename Serializer, typename DataType, typename... Args>
    [[deprecated]] void Serialize(Message& msg, DataType&& data, Args&&... args) const
    {
        Serializer().Serialize(msg, std::forward<DataType>(data), std::forward<Args>(args)...);
    }

    template<typename Deserializer, typename DataType, typename... Args>
    [[deprecated]] void Deserialize(Message& msg, DataType&& data, Args&&... args) const
    {
        Deserializer().Deserialize(msg, std::forward<DataType>(data), std::forward<Args>(args)...);
    }

    /// Shorthand method to send `msg` on `chan` at index `i`
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @param sndTimeoutInMs send timeout in ms, -1 will wait forever (or until interrupt (e.g. via
    /// state change)), 0 will not wait (return immediately if cannot send)
    /// @return Number of bytes that have been queued, TransferCode::timeout if timed out,
    /// TransferCode::error if there was an error, TransferCode::interrupted if interrupted (e.g. by
    /// requested state change)
    int64_t Send(MessagePtr& msg,
                 const std::string& channel,
                 const int index = 0,
                 int sndTimeoutInMs = -1)
    {
        return GetChannel(channel, index).Send(msg, sndTimeoutInMs);
    }

    /// Shorthand method to receive `msg` on `chan` at index `i`
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @param rcvTimeoutInMs receive timeout in ms, -1 will wait forever (or until interrupt (e.g.
    /// via state change)), 0 will not wait (return immediately if cannot receive)
    /// @return Number of bytes that have been received, TransferCode::timeout if timed out,
    /// TransferCode::error if there was an error, TransferCode::interrupted if interrupted (e.g. by
    /// requested state change)
    int64_t Receive(MessagePtr& msg,
                    const std::string& channel,
                    const int index = 0,
                    int rcvTimeoutInMs = -1)
    {
        return GetChannel(channel, index).Receive(msg, rcvTimeoutInMs);
    }

    /// Shorthand method to send Parts on `chan` at index `i`
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @param sndTimeoutInMs send timeout in ms, -1 will wait forever (or until interrupt (e.g. via
    /// state change)), 0 will not wait (return immediately if cannot send)
    /// @return Number of bytes that have been queued, TransferCode::timeout if timed out,
    /// TransferCode::error if there was an error, TransferCode::interrupted if interrupted (e.g. by
    /// requested state change)
    int64_t Send(Parts& parts,
                 const std::string& channel,
                 const int index = 0,
                 int sndTimeoutInMs = -1)
    {
        return GetChannel(channel, index).Send(parts.fParts, sndTimeoutInMs);
    }

    /// Shorthand method to receive Parts on `chan` at index `i`
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @param rcvTimeoutInMs receive timeout in ms, -1 will wait forever (or until interrupt (e.g.
    /// via state change)), 0 will not wait (return immediately if cannot receive)
    /// @return Number of bytes that have been received, TransferCode::timeout if timed out,
    /// TransferCode::error if there was an error, TransferCode::interrupted if interrupted (e.g. by
    /// requested state change)
    int64_t Receive(Parts& parts,
                    const std::string& channel,
                    const int index = 0,
                    int rcvTimeoutInMs = -1)
    {
        return GetChannel(channel, index).Receive(parts.fParts, rcvTimeoutInMs);
    }

    /// @brief Getter for default transport factory
    auto Transport() const -> TransportFactory* { return fTransportFactory.get(); }

    // creates message with the default device transport
    template<typename... Args>
    MessagePtr NewMessage(Args&&... args)
    {
        return Transport()->CreateMessage(std::forward<Args>(args)...);
    }

    // creates message with the transport of the specified channel
    template<typename... Args>
    MessagePtr NewMessageFor(const std::string& channel, int index, Args&&... args)
    {
        return GetChannel(channel, index).NewMessage(std::forward<Args>(args)...);
    }

    // creates a message that will not be cleaned up after transfer, with the default device
    // transport
    template<typename T>
    MessagePtr NewStaticMessage(const T& data)
    {
        return Transport()->NewStaticMessage(data);
    }

    // creates a message that will not be cleaned up after transfer, with the transport of the
    // specified channel
    template<typename T>
    MessagePtr NewStaticMessageFor(const std::string& channel, int index, const T& data)
    {
        return GetChannel(channel, index).NewStaticMessage(data);
    }

    // creates a message with a copy of the provided data, with the default device transport
    template<typename T>
    MessagePtr NewSimpleMessage(const T& data)
    {
        return Transport()->NewSimpleMessage(data);
    }

    // creates a message with a copy of the provided data, with the transport of the specified
    // channel
    template<typename T>
    MessagePtr NewSimpleMessageFor(const std::string& channel, int index, const T& data)
    {
        return GetChannel(channel, index).NewSimpleMessage(data);
    }

    // creates unamanaged region with the default device transport
    template<typename... Args>
    UnmanagedRegionPtr NewUnmanagedRegion(Args&&... args)
    {
        return Transport()->CreateUnmanagedRegion(std::forward<Args>(args)...);
    }

    // creates unmanaged region with the transport of the specified channel
    template<typename... Args>
    UnmanagedRegionPtr NewUnmanagedRegionFor(const std::string& channel, int index, Args&&... args)
    {
        return GetChannel(channel, index).NewUnmanagedRegion(std::forward<Args>(args)...);
    }

    template<typename... Ts>
    PollerPtr NewPoller(const Ts&... inputs)
    {
        std::vector<std::string> chans{inputs...};

        // if more than one channel provided, check compatibility
        if (chans.size() > 1) {
            mq::Transport type = GetChannel(chans.at(0), 0).Transport()->GetType();

            for (unsigned int i = 1; i < chans.size(); ++i) {
                if (type != GetChannel(chans.at(i), 0).Transport()->GetType()) {
                    LOG(error) << "poller failed: different transports within same poller are not "
                                  "yet supported. Going to ERROR state.";
                    throw std::runtime_error("poller failed: different transports within same "
                                             "poller are not yet supported.");
                }
            }
        }

        return GetChannel(chans.at(0), 0).Transport()->CreatePoller(fChannels, chans);
    }

    PollerPtr NewPoller(const std::vector<Channel*>& channels)
    {
        // if more than one channel provided, check compatibility
        if (channels.size() > 1) {
            mq::Transport type = channels.at(0)->Transport()->GetType();

            for (unsigned int i = 1; i < channels.size(); ++i) {
                if (type != channels.at(i)->Transport()->GetType()) {
                    LOG(error) << "poller failed: different transports within same poller are not "
                                  "yet supported. Going to ERROR state.";
                    throw std::runtime_error("poller failed: different transports within same "
                                             "poller are not yet supported.");
                }
            }
        }

        return channels.at(0)->Transport()->CreatePoller(channels);
    }

    /// Adds a transport to the device if it doesn't exist
    /// @param transport  Transport string ("zeromq"/"shmem")
    std::shared_ptr<TransportFactory> AddTransport(mq::Transport transport);

    /// Assigns config to the device
    void SetConfig(ProgOptions& config);
    /// Get pointer to the config
    ProgOptions* GetConfig() const { return fConfig; }

    // overload to easily bind member functions
    template<typename T>
    void OnData(const std::string& channelName,
                bool (T::*memberFunction)(MessagePtr& msg, int index))
    {
        fDataCallbacks = true;
        fMsgInputs.insert(
            std::make_pair(channelName, [this, memberFunction](MessagePtr& msg, int index) {
                return (static_cast<T*>(this)->*memberFunction)(msg, index);
            }));

        if (find(fInputChannelKeys.begin(), fInputChannelKeys.end(), channelName)
            == fInputChannelKeys.end()) {
            fInputChannelKeys.push_back(channelName);
        }
    }

    void OnData(const std::string& channelName, InputMsgCallback callback)
    {
        fDataCallbacks = true;
        fMsgInputs.insert(make_pair(channelName, callback));

        if (find(fInputChannelKeys.begin(), fInputChannelKeys.end(), channelName)
            == fInputChannelKeys.end()) {
            fInputChannelKeys.push_back(channelName);
        }
    }

    // overload to easily bind member functions
    template<typename T>
    void OnData(const std::string& channelName, bool (T::*memberFunction)(Parts& parts, int index))
    {
        fDataCallbacks = true;
        fMultipartInputs.insert(
            std::make_pair(channelName, [this, memberFunction](Parts& parts, int index) {
                return (static_cast<T*>(this)->*memberFunction)(parts, index);
            }));

        if (find(fInputChannelKeys.begin(), fInputChannelKeys.end(), channelName)
            == fInputChannelKeys.end()) {
            fInputChannelKeys.push_back(channelName);
        }
    }

    void OnData(const std::string& channelName, InputMultipartCallback callback)
    {
        fDataCallbacks = true;
        fMultipartInputs.insert(make_pair(channelName, callback));

        if (find(fInputChannelKeys.begin(), fInputChannelKeys.end(), channelName)
            == fInputChannelKeys.end()) {
            fInputChannelKeys.push_back(channelName);
        }
    }

    Channel& GetChannel(const std::string& channelName, const int index = 0)
    try {
        return fChannels.at(channelName).at(index);
    } catch (const std::out_of_range& oor) {
        LOG(error)
            << "requested channel has not been configured? check channel names/configuration.";
        LOG(error) << "channel: " << channelName << ", index: " << index;
        LOG(error) << "out of range: " << oor.what();
        throw;
    }

    virtual void RegisterChannelEndpoints() {}

    bool RegisterChannelEndpoint(const std::string& channelName,
                                 uint16_t minNumSubChannels = 1,
                                 uint16_t maxNumSubChannels = 1)
    {
        bool ok = fChannelRegistry
                      .insert(std::make_pair(channelName,
                                             std::make_pair(minNumSubChannels, maxNumSubChannels)))
                      .second;
        if (!ok) {
            LOG(warn) << "Registering channel: name already registered: \"" << channelName << "\"";
        }
        return ok;
    }

    void PrintRegisteredChannels()
    {
        if (fChannelRegistry.empty()) {
            LOGV(info, verylow) << "no channels registered.";
        } else {
            for (const auto& c : fChannelRegistry) {
                LOGV(info, verylow) << c.first << ":" << c.second.first << ":" << c.second.second;
            }
        }
    }

    void SetId(const std::string& id) { fId = id; }
    std::string GetId() { return fId; }

    const tools::Version GetVersion() const { return fVersion; }

    void SetNumIoThreads(int numIoThreads) { fConfig->SetProperty("io-threads", numIoThreads); }
    int GetNumIoThreads() const
    {
        return fConfig->GetProperty<int>("io-threads", DefaultIOThreads);
    }

    void SetNetworkInterface(const std::string& networkInterface)
    {
        fConfig->SetProperty("network-interface", networkInterface);
    }
    std::string GetNetworkInterface() const
    {
        return fConfig->GetProperty<std::string>("network-interface", DefaultNetworkInterface);
    }

    void SetDefaultTransport(const std::string& name) { fConfig->SetProperty("transport", name); }
    std::string GetDefaultTransport() const
    {
        return fConfig->GetProperty<std::string>("transport", DefaultTransportName);
    }

    void SetInitTimeoutInS(int initTimeoutInS)
    {
        fConfig->SetProperty("init-timeout", initTimeoutInS);
    }
    int GetInitTimeoutInS() const
    {
        return fConfig->GetProperty<int>("init-timeout", DefaultInitTimeout);
    }

    /// Sets the default transport for the device
    /// @param transport  Transport string ("zeromq"/"shmem")
    void SetTransport(const std::string& transport)
    {
        fConfig->SetProperty("transport", transport);
    }
    /// Gets the default transport name
    std::string GetTransportName() const
    {
        return fConfig->GetProperty<std::string>("transport", DefaultTransportName);
    }

    void SetRawCmdLineArgs(const std::vector<std::string>& args) { fRawCmdLineArgs = args; }
    std::vector<std::string> GetRawCmdLineArgs() const { return fRawCmdLineArgs; }

    void RunStateMachine() { fStateMachine.ProcessWork(); };

    /// Wait for the supplied amount of time or for interruption.
    /// If interrupted, returns false, otherwise true.
    /// @param duration wait duration
    template<typename Rep, typename Period>
    bool WaitFor(std::chrono::duration<Rep, Period> const& duration)
    {
        return !fStateMachine.WaitForPendingStateFor(
            std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }

  protected:
    std::shared_ptr<TransportFactory> fTransportFactory;   ///< Default transport factory
    std::unordered_map<mq::Transport, std::shared_ptr<TransportFactory>>
        fTransports;   ///< Container for transports

  public:
    std::unordered_map<std::string, std::vector<Channel>> fChannels;   ///< Device channels
    std::unique_ptr<ProgOptions> fInternalConfig;   ///< Internal program options configuration
    ProgOptions* fConfig;                           ///< Pointer to config (internal or external)

    void AddChannel(const std::string& name, Channel&& channel)
    {
        fConfig->AddChannel(name, channel);
    }

  protected:
    std::string fId;   ///< Device ID

    /// Additional user initialization (can be overloaded in child classes). Prefer to use
    /// InitTask().
    virtual void Init() {}

    virtual void Bind() {}

    virtual void Connect() {}

    /// Task initialization (can be overloaded in child classes)
    virtual void InitTask() {}

    /// Runs the device (to be overloaded in child classes)
    virtual void Run() {}

    /// Called in the RUNNING state once before executing the Run()/ConditionalRun() method
    virtual void PreRun() {}

    /// Called during RUNNING state repeatedly until it returns false or device state changes
    virtual bool ConditionalRun() { return false; }

    /// Called in the RUNNING state once after executing the Run()/ConditionalRun() method
    virtual void PostRun() {}

    /// Resets the user task (to be overloaded in child classes)
    virtual void ResetTask() {}

    /// Resets the device (can be overloaded in child classes)
    virtual void Reset() {}

  public:
    /// @brief Request a device state transition
    /// @param transition state transition
    ///
    /// The state transition may not happen immediately, but when the current state evaluates the
    /// pending transition event and terminates. In other words, the device states are scheduled
    /// cooperatively.
    bool ChangeState(const Transition transition) { return fStateMachine.ChangeState(transition); }
    /// @brief Request a device state transition
    /// @param transition state transition
    ///
    /// The state transition may not happen immediately, but when the current state evaluates the
    /// pending transition event and terminates. In other words, the device states are scheduled
    /// cooperatively.
    bool ChangeState(const std::string& transition)
    {
        return fStateMachine.ChangeState(GetTransition(transition));
    }

    /// @brief waits for the next state (any) to occur
    State WaitForNextState() { return fStateQueue.WaitForNext(); }
    /// @brief waits for the specified state to occur
    /// @param state state to wait for
    void WaitForState(State state) { fStateQueue.WaitForState(state); }
    /// @brief waits for the specified state to occur
    /// @param state state to wait for
    void WaitForState(const std::string& state) { WaitForState(GetState(state)); }

    void TransitionTo(State state);

    /// @brief Subscribe with a callback to state changes
    /// @param key id to identify your subscription
    /// @param callback callback (called with the new state as the parameter)
    ///
    /// The callback is called at the beginning of a new state.
    /// The callback is called from the thread the state is running in.
    void SubscribeToStateChange(const std::string& key, std::function<void(const State)> callback)
    {
        fStateMachine.SubscribeToStateChange(key, callback);
    }
    /// @brief Unsubscribe from state changes
    /// @param key id (that was used when subscribing)
    void UnsubscribeFromStateChange(const std::string& key)
    {
        fStateMachine.UnsubscribeFromStateChange(key);
    }

    /// @brief Subscribe with a callback to incoming state transitions
    /// @param key id to identify your subscription
    /// @param callback callback (called with the incoming transition as the parameter)
    /// The callback is called when new transition is initiated.
    /// The callback is called from the thread that initiates the transition (via ChangeState).
    void SubscribeToNewTransition(const std::string& key,
                                  std::function<void(const Transition)> callback)
    {
        fStateMachine.SubscribeToNewTransition(key, callback);
    }
    /// @brief Unsubscribe from state transitions
    /// @param key id (that was used when subscribing)
    void UnsubscribeFromNewTransition(const std::string& key)
    {
        fStateMachine.UnsubscribeFromNewTransition(key);
    }

    /// @brief Returns true if a new state has been requested, signaling the current handler to
    /// stop.
    bool NewStatePending() const { return fStateMachine.NewStatePending(); }

    /// @brief Returns the current state
    State GetCurrentState() const { return fStateMachine.GetCurrentState(); }
    /// @brief Returns the name of the current state as a string
    std::string GetCurrentStateName() const { return fStateMachine.GetCurrentStateName(); }

    /// @brief Returns name of the given state as a string
    /// @param state state
    [[deprecated("Use fair::mq::GetStateName from <fairmq/States.h> directly")]]
    static std::string GetStateName(State state) { return fair::mq::GetStateName(state); }
    /// @brief Returns name of the given transition as a string
    /// @param transition transition
    [[deprecated("Use fair::mq::GetTransitionName from <fairmq/States.h> directly")]]
    static std::string GetTransitionName(Transition transition)
    {
        return fair::mq::GetTransitionName(transition);
    }

    static constexpr const char* DefaultId = "";
    static constexpr int DefaultIOThreads = 1;
    static constexpr const char* DefaultTransportName = "zeromq";
    static constexpr mq::Transport DefaultTransportType = mq::Transport::ZMQ;
    static constexpr const char* DefaultNetworkInterface = "default";
    static constexpr int DefaultInitTimeout = 120;
    static constexpr uint64_t DefaultMaxRunTime = 0;
    static constexpr float DefaultRate = 0.;
    static constexpr const char* DefaultSession = "default";

  private:
    mq::Transport fDefaultTransportType;   ///< Default transport for the device
    StateMachine fStateMachine;

    /// Handles the initialization
    void InitWrapper();
    /// Initializes binding channels
    void BindWrapper();
    /// Initializes connecting channels
    void ConnectWrapper();
    /// Handles the InitTask() method
    void InitTaskWrapper();
    /// Handles the Run() method
    void RunWrapper();
    /// Handles the ResetTask() method
    void ResetTaskWrapper();
    /// Handles the Reset() method
    void ResetWrapper();

    /// Notifies transports to cease any blocking activity
    void UnblockTransports();

    /// Shuts down the transports and the device
    void Exit() {}

    /// Attach (bind/connect) channels in the list
    void AttachChannels(std::vector<Channel*>& chans);
    bool AttachChannel(Channel& ch);

    void HandleSingleChannelInput();
    void HandleMultipleChannelInput();
    void HandleMultipleTransportInput();
    void PollForTransport(const TransportFactory* factory,
                          const std::vector<std::string>& channelKeys);

    bool HandleMsgInput(const std::string& chName, const InputMsgCallback& callback, int i);
    bool HandleMultipartInput(const std::string& chName,
                              const InputMultipartCallback& callback,
                              int i);

    std::vector<Channel*> fUninitializedBindingChannels;
    std::vector<Channel*> fUninitializedConnectingChannels;

    bool fDataCallbacks;
    std::unordered_map<std::string, InputMsgCallback> fMsgInputs;
    std::unordered_map<std::string, InputMultipartCallback> fMultipartInputs;
    std::unordered_map<mq::Transport, std::vector<std::string>> fMultitransportInputs;
    std::unordered_map<std::string, std::pair<uint16_t, uint16_t>> fChannelRegistry;
    std::vector<std::string> fInputChannelKeys;
    std::mutex fMultitransportMutex;
    std::atomic<bool> fMultitransportProceed;

    const tools::Version fVersion;
    float fRate;                  ///< Rate limiting for ConditionalRun
    uint64_t fMaxRunRuntimeInS;   ///< Maximum runtime for the Running state handler, after which
                                  ///< state will change to Ready (in seconds, 0 for no limit).
    int fInitializationTimeoutInS;
    std::vector<std::string> fRawCmdLineArgs;

    StateQueue fStateQueue;

    std::mutex fTransitionMtx;
    bool fTransitioning;
};

}   // namespace fair::mq

// using FairMQChannelMap [[deprecated("Use fair::mq::ChannelMap")]] = fair::mq::ChannelMap;
// using InputMsgCallback [[deprecated("Use fair::mq::InputMsgCallback")]] =
    // fair::mq::InputMsgCallback;
// using InputMultipartCallback [[deprecated("Use fair::mq::InputMultipartCallback")]] =
    // fair::mq::InputMultipartCallback;
// using FairMQDevice [[deprecated("Use fair::mq::Device")]] = fair::mq::Device;
using FairMQChannelMap = fair::mq::ChannelMap;
using InputMsgCallback = fair::mq::InputMsgCallback;
using InputMultipartCallback = fair::mq::InputMultipartCallback;
using FairMQDevice = fair::mq::Device;

#endif /* FAIR_MQ_DEVICE_H */

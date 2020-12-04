/********************************************************************************
 * Copyright (C) 2012-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQDEVICE_H_
#define FAIRMQDEVICE_H_

#include <StateMachine.h>
#include <FairMQTransportFactory.h>
#include <fairmq/Transports.h>
#include <fairmq/StateQueue.h>

#include <FairMQChannel.h>
#include <FairMQMessage.h>
#include <FairMQParts.h>
#include <FairMQUnmanagedRegion.h>
#include <FairMQLogger.h>
#include <fairmq/ProgOptions.h>

#include <vector>
#include <memory> // unique_ptr
#include <algorithm> // find
#include <string>
#include <chrono>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <mutex>
#include <atomic>
#include <cstddef>
#include <utility> // pair

#include <fairmq/tools/Version.h>

using FairMQChannelMap = std::unordered_map<std::string, std::vector<FairMQChannel>>;

using InputMsgCallback = std::function<bool(FairMQMessagePtr&, int)>;
using InputMultipartCallback = std::function<bool(FairMQParts&, int)>;

namespace fair
{
namespace mq
{
struct OngoingTransition : std::runtime_error { using std::runtime_error::runtime_error; };
}
}

class FairMQDevice
{
    friend class FairMQChannel;

  public:
    /// Default constructor
    FairMQDevice();
    /// Constructor with external fair::mq::ProgOptions
    FairMQDevice(fair::mq::ProgOptions& config);

    /// Constructor that sets the version
    FairMQDevice(const fair::mq::tools::Version version);

    /// Constructor that sets the version and external fair::mq::ProgOptions
    FairMQDevice(fair::mq::ProgOptions& config, const fair::mq::tools::Version version);

  private:
    FairMQDevice(fair::mq::ProgOptions* config, const fair::mq::tools::Version version);

  public:
    /// Copy constructor (disabled)
    FairMQDevice(const FairMQDevice&) = delete;
    /// Assignment operator (disabled)
    FairMQDevice operator=(const FairMQDevice&) = delete;
    /// Default destructor
    virtual ~FairMQDevice();

    /// Outputs the socket transfer rates
    virtual void LogSocketRates();

    template<typename Serializer, typename DataType, typename... Args>
    void Serialize(FairMQMessage& msg, DataType&& data, Args&&... args) const
    {
        Serializer().Serialize(msg, std::forward<DataType>(data), std::forward<Args>(args)...);
    }

    template<typename Deserializer, typename DataType, typename... Args>
    void Deserialize(FairMQMessage& msg, DataType&& data, Args&&... args) const
    {
        Deserializer().Deserialize(msg, std::forward<DataType>(data), std::forward<Args>(args)...);
    }

    /// Shorthand method to send `msg` on `chan` at index `i`
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @param sndTimeoutInMs send timeout in ms, -1 will wait forever (or until interrupt (e.g. via state change)), 0 will not wait (return immediately if cannot send)
    /// @return Number of bytes that have been queued, TransferResult::timeout if timed out, TransferResult::error if there was an error, TransferResult::interrupted if interrupted (e.g. by requested state change)
    int64_t Send(FairMQMessagePtr& msg, const std::string& channel, const int index = 0, int sndTimeoutInMs = -1)
    {
        return GetChannel(channel, index).Send(msg, sndTimeoutInMs);
    }

    /// Shorthand method to receive `msg` on `chan` at index `i`
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @param rcvTimeoutInMs receive timeout in ms, -1 will wait forever (or until interrupt (e.g. via state change)), 0 will not wait (return immediately if cannot receive)
    /// @return Number of bytes that have been received, TransferResult::timeout if timed out, TransferResult::error if there was an error, TransferResult::interrupted if interrupted (e.g. by requested state change)
    int64_t Receive(FairMQMessagePtr& msg, const std::string& channel, const int index = 0, int rcvTimeoutInMs = -1)
    {
        return GetChannel(channel, index).Receive(msg, rcvTimeoutInMs);
    }

    /// Shorthand method to send FairMQParts on `chan` at index `i`
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @param sndTimeoutInMs send timeout in ms, -1 will wait forever (or until interrupt (e.g. via state change)), 0 will not wait (return immediately if cannot send)
    /// @return Number of bytes that have been queued, TransferResult::timeout if timed out, TransferResult::error if there was an error, TransferResult::interrupted if interrupted (e.g. by requested state change)
    int64_t Send(FairMQParts& parts, const std::string& channel, const int index = 0, int sndTimeoutInMs = -1)
    {
        return GetChannel(channel, index).Send(parts.fParts, sndTimeoutInMs);
    }

    /// Shorthand method to receive FairMQParts on `chan` at index `i`
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @param rcvTimeoutInMs receive timeout in ms, -1 will wait forever (or until interrupt (e.g. via state change)), 0 will not wait (return immediately if cannot receive)
    /// @return Number of bytes that have been received, TransferResult::timeout if timed out, TransferResult::error if there was an error, TransferResult::interrupted if interrupted (e.g. by requested state change)
    int64_t Receive(FairMQParts& parts, const std::string& channel, const int index = 0, int rcvTimeoutInMs = -1)
    {
        return GetChannel(channel, index).Receive(parts.fParts, rcvTimeoutInMs);
    }

    /// @brief Getter for default transport factory
    auto Transport() const -> FairMQTransportFactory*
    {
        return fTransportFactory.get();
    }

    // creates message with the default device transport
    template<typename... Args>
    FairMQMessagePtr NewMessage(Args&&... args)
    {
        return Transport()->CreateMessage(std::forward<Args>(args)...);
    }

    // creates message with the transport of the specified channel
    template<typename... Args>
    FairMQMessagePtr NewMessageFor(const std::string& channel, int index, Args&&... args)
    {
        return GetChannel(channel, index).NewMessage(std::forward<Args>(args)...);
    }

    // creates a message that will not be cleaned up after transfer, with the default device transport
    template<typename T>
    FairMQMessagePtr NewStaticMessage(const T& data)
    {
        return Transport()->NewStaticMessage(data);
    }

    // creates a message that will not be cleaned up after transfer, with the transport of the specified channel
    template<typename T>
    FairMQMessagePtr NewStaticMessageFor(const std::string& channel, int index, const T& data)
    {
        return GetChannel(channel, index).NewStaticMessage(data);
    }

    // creates a message with a copy of the provided data, with the default device transport
    template<typename T>
    FairMQMessagePtr NewSimpleMessage(const T& data)
    {
        return Transport()->NewSimpleMessage(data);
    }

    // creates a message with a copy of the provided data, with the transport of the specified channel
    template<typename T>
    FairMQMessagePtr NewSimpleMessageFor(const std::string& channel, int index, const T& data)
    {
        return GetChannel(channel, index).NewSimpleMessage(data);
    }

    // creates unamanaged region with the default device transport
    template<typename... Args>
    FairMQUnmanagedRegionPtr NewUnmanagedRegion(Args&&... args)
    {
        return Transport()->CreateUnmanagedRegion(std::forward<Args>(args)...);
    }

    // creates unmanaged region with the transport of the specified channel
    template<typename... Args>
    FairMQUnmanagedRegionPtr NewUnmanagedRegionFor(const std::string& channel, int index, Args&&... args)
    {
        return GetChannel(channel, index).NewUnmanagedRegion(std::forward<Args>(args)...);
    }

    template<typename ...Ts>
    FairMQPollerPtr NewPoller(const Ts&... inputs)
    {
        std::vector<std::string> chans{inputs...};

        // if more than one channel provided, check compatibility
        if (chans.size() > 1)
        {
            fair::mq::Transport type = GetChannel(chans.at(0), 0).Transport()->GetType();

            for (unsigned int i = 1; i < chans.size(); ++i)
            {
                if (type != GetChannel(chans.at(i), 0).Transport()->GetType())
                {
                    LOG(error) << "poller failed: different transports within same poller are not yet supported. Going to ERROR state.";
                    throw std::runtime_error("poller failed: different transports within same poller are not yet supported.");
                }
            }
        }

        return GetChannel(chans.at(0), 0).Transport()->CreatePoller(fChannels, chans);
    }

    FairMQPollerPtr NewPoller(const std::vector<FairMQChannel*>& channels)
    {
        // if more than one channel provided, check compatibility
        if (channels.size() > 1)
        {
            fair::mq::Transport type = channels.at(0)->Transport()->GetType();

            for (unsigned int i = 1; i < channels.size(); ++i)
            {
                if (type != channels.at(i)->Transport()->GetType())
                {
                    LOG(error) << "poller failed: different transports within same poller are not yet supported. Going to ERROR state.";
                    throw std::runtime_error("poller failed: different transports within same poller are not yet supported.");
                }
            }
        }

        return channels.at(0)->Transport()->CreatePoller(channels);
    }

    /// Adds a transport to the device if it doesn't exist
    /// @param transport  Transport string ("zeromq"/"shmem")
    std::shared_ptr<FairMQTransportFactory> AddTransport(const fair::mq::Transport transport);

    /// Assigns config to the device
    void SetConfig(fair::mq::ProgOptions& config);
    /// Get pointer to the config
    fair::mq::ProgOptions* GetConfig() const
    {
        return fConfig;
    }

    // overload to easily bind member functions
    template<typename T>
    void OnData(const std::string& channelName, bool (T::* memberFunction)(FairMQMessagePtr& msg, int index))
    {
        fDataCallbacks = true;
        fMsgInputs.insert(std::make_pair(channelName, [this, memberFunction](FairMQMessagePtr& msg, int index)
        {
            return (static_cast<T*>(this)->*memberFunction)(msg, index);
        }));

        if (find(fInputChannelKeys.begin(), fInputChannelKeys.end(), channelName) == fInputChannelKeys.end())
        {
            fInputChannelKeys.push_back(channelName);
        }
    }

    void OnData(const std::string& channelName, InputMsgCallback callback)
    {
        fDataCallbacks = true;
        fMsgInputs.insert(make_pair(channelName, callback));

        if (find(fInputChannelKeys.begin(), fInputChannelKeys.end(), channelName) == fInputChannelKeys.end())
        {
            fInputChannelKeys.push_back(channelName);
        }
    }

    // overload to easily bind member functions
    template<typename T>
    void OnData(const std::string& channelName, bool (T::* memberFunction)(FairMQParts& parts, int index))
    {
        fDataCallbacks = true;
        fMultipartInputs.insert(std::make_pair(channelName, [this, memberFunction](FairMQParts& parts, int index)
        {
            return (static_cast<T*>(this)->*memberFunction)(parts, index);
        }));

        if (find(fInputChannelKeys.begin(), fInputChannelKeys.end(), channelName) == fInputChannelKeys.end())
        {
            fInputChannelKeys.push_back(channelName);
        }
    }

    void OnData(const std::string& channelName, InputMultipartCallback callback)
    {
        fDataCallbacks = true;
        fMultipartInputs.insert(make_pair(channelName, callback));

        if (find(fInputChannelKeys.begin(), fInputChannelKeys.end(), channelName) == fInputChannelKeys.end())
        {
            fInputChannelKeys.push_back(channelName);
        }
    }

    FairMQChannel& GetChannel(const std::string& channelName, const int index = 0)
    try {
        return fChannels.at(channelName).at(index);
    } catch (const std::out_of_range& oor) {
        LOG(error) << "requested channel has not been configured? check channel names/configuration.";
        LOG(error) << "channel: " << channelName << ", index: " << index;
        LOG(error) << "out of range: " << oor.what();
        throw;
    }

    virtual void RegisterChannelEndpoints() {}

    bool RegisterChannelEndpoint(const std::string& channelName, uint16_t minNumSubChannels = 1, uint16_t maxNumSubChannels = 1)
    {
        bool ok = fChannelRegistry.insert(std::make_pair(channelName, std::make_pair(minNumSubChannels, maxNumSubChannels))).second;
        if (!ok) {
            LOG(warn) << "Registering channel: name already registered: \"" << channelName << "\"";
        }
        return ok;
    }

    void PrintRegisteredChannels()
    {
        if (fChannelRegistry.size() < 1) {
            LOGV(info, verylow) << "no channels registered.";
        } else {
            for (const auto& c : fChannelRegistry) {
                LOGV(info, verylow) << c.first << ":" << c.second.first << ":" << c.second.second;
            }
        }
    }

    void SetId(const std::string& id) { fId = id; }
    std::string GetId() { return fId; }

    const fair::mq::tools::Version GetVersion() const { return fVersion; }

    void SetNumIoThreads(int numIoThreads) { fConfig->SetProperty("io-threads", numIoThreads);}
    int GetNumIoThreads() const { return fConfig->GetProperty<int>("io-threads", DefaultIOThreads); }

    void SetNetworkInterface(const std::string& networkInterface) { fConfig->SetProperty("network-interface", networkInterface); }
    std::string GetNetworkInterface() const { return fConfig->GetProperty<std::string>("network-interface", DefaultNetworkInterface); }

    void SetDefaultTransport(const std::string& name) { fConfig->SetProperty("transport", name); }
    std::string GetDefaultTransport() const { return fConfig->GetProperty<std::string>("transport", DefaultTransportName); }

    void SetInitTimeoutInS(int initTimeoutInS) { fConfig->SetProperty("init-timeout", initTimeoutInS); }
    int GetInitTimeoutInS() const { return fConfig->GetProperty<int>("init-timeout", DefaultInitTimeout); }

    /// Sets the default transport for the device
    /// @param transport  Transport string ("zeromq"/"shmem")
    void SetTransport(const std::string& transport) { fConfig->SetProperty("transport", transport); }
    /// Gets the default transport name
    std::string GetTransportName() const { return fConfig->GetProperty<std::string>("transport", DefaultTransportName); }

    void SetRawCmdLineArgs(const std::vector<std::string>& args) { fRawCmdLineArgs = args; }
    std::vector<std::string> GetRawCmdLineArgs() const { return fRawCmdLineArgs; }

    void RunStateMachine()
    {
        fStateMachine.ProcessWork();
    };

    /// Wait for the supplied amount of time or for interruption.
    /// If interrupted, returns false, otherwise true.
    /// @param duration wait duration
    template<typename Rep, typename Period>
    bool WaitFor(std::chrono::duration<Rep, Period> const& duration)
    {
        return !fStateMachine.WaitForPendingStateFor(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }

  protected:
    std::shared_ptr<FairMQTransportFactory> fTransportFactory; ///< Default transport factory
    std::unordered_map<fair::mq::Transport, std::shared_ptr<FairMQTransportFactory>> fTransports; ///< Container for transports

  public:
    std::unordered_map<std::string, std::vector<FairMQChannel>> fChannels; ///< Device channels
    std::unique_ptr<fair::mq::ProgOptions> fInternalConfig; ///< Internal program options configuration
    fair::mq::ProgOptions* fConfig; ///< Pointer to config (internal or external)

    void AddChannel(const std::string& name, FairMQChannel&& channel)
    {
        fConfig->AddChannel(name, channel);
    }

  protected:
    std::string fId; ///< Device ID

    /// Additional user initialization (can be overloaded in child classes). Prefer to use InitTask().
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
    bool ChangeState(const fair::mq::Transition transition) { return fStateMachine.ChangeState(transition); }
    bool ChangeState(const std::string& transition) { return fStateMachine.ChangeState(fair::mq::GetTransition(transition)); }

    fair::mq::State WaitForNextState() { return fStateQueue.WaitForNext(); }
    void WaitForState(fair::mq::State state) { fStateQueue.WaitForState(state); }
    void WaitForState(const std::string& state) { WaitForState(fair::mq::GetState(state)); }

    void TransitionTo(const fair::mq::State state);

    void SubscribeToStateChange(const std::string& key, std::function<void(const fair::mq::State)> callback) { fStateMachine.SubscribeToStateChange(key, callback); }
    void UnsubscribeFromStateChange(const std::string& key) { fStateMachine.UnsubscribeFromStateChange(key); }

    void SubscribeToNewTransition(const std::string& key, std::function<void(const fair::mq::Transition)> callback) { fStateMachine.SubscribeToNewTransition(key, callback); }
    void UnsubscribeFromNewTransition(const std::string& key) { fStateMachine.UnsubscribeFromNewTransition(key); }

    /// Returns true is a new state has been requested, signaling the current handler to stop.
    bool NewStatePending() const { return fStateMachine.NewStatePending(); }

    fair::mq::State GetCurrentState() const { return fStateMachine.GetCurrentState(); }
    std::string GetCurrentStateName() const { return fStateMachine.GetCurrentStateName(); }

    static std::string GetStateName(const fair::mq::State state) { return fair::mq::GetStateName(state); }
    static std::string GetTransitionName(const fair::mq::Transition transition) { return fair::mq::GetTransitionName(transition); }

    static constexpr const char* DefaultId = "";
    static constexpr int DefaultIOThreads = 1;
    static constexpr const char* DefaultTransportName = "zeromq";
    static constexpr fair::mq::Transport DefaultTransportType = fair::mq::Transport::ZMQ;
    static constexpr const char* DefaultNetworkInterface = "default";
    static constexpr int DefaultInitTimeout = 120;
    static constexpr uint64_t DefaultMaxRunTime = 0;
    static constexpr float DefaultRate = 0.;
    static constexpr const char* DefaultSession = "default";

  private:
    fair::mq::Transport fDefaultTransportType; ///< Default transport for the device
    fair::mq::StateMachine fStateMachine;

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
    void AttachChannels(std::vector<FairMQChannel*>& chans);
    bool AttachChannel(FairMQChannel& ch);

    void HandleSingleChannelInput();
    void HandleMultipleChannelInput();
    void HandleMultipleTransportInput();
    void PollForTransport(const FairMQTransportFactory* factory, const std::vector<std::string>& channelKeys);

    bool HandleMsgInput(const std::string& chName, const InputMsgCallback& callback, int i);
    bool HandleMultipartInput(const std::string& chName, const InputMultipartCallback& callback, int i);

    std::vector<FairMQChannel*> fUninitializedBindingChannels;
    std::vector<FairMQChannel*> fUninitializedConnectingChannels;

    bool fDataCallbacks;
    std::unordered_map<std::string, InputMsgCallback> fMsgInputs;
    std::unordered_map<std::string, InputMultipartCallback> fMultipartInputs;
    std::unordered_map<fair::mq::Transport, std::vector<std::string>> fMultitransportInputs;
    std::unordered_map<std::string, std::pair<uint16_t, uint16_t>> fChannelRegistry;
    std::vector<std::string> fInputChannelKeys;
    std::mutex fMultitransportMutex;
    std::atomic<bool> fMultitransportProceed;

    const fair::mq::tools::Version fVersion;
    float fRate; ///< Rate limiting for ConditionalRun
    uint64_t fMaxRunRuntimeInS; ///< Maximum runtime for the Running state handler, after which state will change to Ready (in seconds, 0 for no limit).
    int fInitializationTimeoutInS;
    std::vector<std::string> fRawCmdLineArgs;

    fair::mq::StateQueue fStateQueue;

    std::mutex fTransitionMtx;
    bool fTransitioning;
};

#endif /* FAIRMQDEVICE_H_ */

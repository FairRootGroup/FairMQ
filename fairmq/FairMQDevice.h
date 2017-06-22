/********************************************************************************
 * Copyright (C) 2012-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQDEVICE_H_
#define FAIRMQDEVICE_H_

#include "FairMQConfigurable.h"
#include "FairMQStateMachine.h"
#include "FairMQTransportFactory.h"
#include "FairMQTransports.h"

#include "FairMQSocket.h"
#include "FairMQChannel.h"
#include "FairMQMessage.h"
#include "FairMQParts.h"

#include <vector>
#include <memory> // unique_ptr
#include <algorithm> // std::sort()
#include <string>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <assert.h> // static_assert
#include <type_traits> // is_trivially_copyable

#include <mutex>
#include <condition_variable>

typedef std::unordered_map<std::string, std::vector<FairMQChannel>> FairMQChannelMap;

typedef std::function<bool(FairMQMessagePtr&, int)> InputMsgCallback;
typedef std::function<bool(FairMQParts&, int)> InputMultipartCallback;

class FairMQProgOptions;

class FairMQDevice : public FairMQStateMachine, public FairMQConfigurable
{
    friend class FairMQChannel;

  public:
    enum
    {
        Id = FairMQConfigurable::Last, ///< Device ID
        NumIoThreads, ///< Number of ZeroMQ I/O threads
        Last
    };

    /// Default constructor
    FairMQDevice();
    /// Copy constructor (disabled)
    FairMQDevice(const FairMQDevice&) = delete;
    /// Assignment operator (disabled)
    FairMQDevice operator=(const FairMQDevice&) = delete;
    /// Default destructor
    virtual ~FairMQDevice();

    /// Catches interrupt signals (SIGINT, SIGTERM)
    void CatchSignals();

    /// Outputs the socket transfer rates
    virtual void LogSocketRates();

    /// Sorts a channel by address, with optional reindexing of the sorted values
    /// @param name    Channel name
    /// @param reindex Should reindexing be done
    void SortChannel(const std::string& name, const bool reindex = true);

    /// Prints channel configuration
    /// @param name Name of the channel
    void PrintChannel(const std::string& name);

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

    inline int Send(FairMQMessagePtr& msg, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Send(msg);
    }

    inline int Receive(FairMQMessagePtr& msg, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Receive(msg);
    }

    /// Shorthand method to send `msg` on `chan` at index `i`
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    inline int Send(FairMQMessagePtr& msg, const std::string& chan, const int i, int sndTimeoutInMs) const
    {
        return fChannels.at(chan).at(i).Send(msg, sndTimeoutInMs);
    }

    /// Shorthand method to receive `msg` on `chan` at index `i`
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    inline int Receive(FairMQMessagePtr& msg, const std::string& chan, const int i, int rcvTimeoutInMs) const
    {
        return fChannels.at(chan).at(i).Receive(msg, rcvTimeoutInMs);
    }

    /// Shorthand method to send `msg` on `chan` at index `i` without blocking
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    inline int SendAsync(FairMQMessagePtr& msg, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).SendAsync(msg);
    }

    /// Shorthand method to receive `msg` on `chan` at index `i` without blocking
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    inline int ReceiveAsync(FairMQMessagePtr& msg, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).ReceiveAsync(msg);
    }

    inline int64_t Send(FairMQParts& parts, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Send(parts.fParts);
    }

    inline int64_t Receive(FairMQParts& parts, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Receive(parts.fParts);
    }

    /// Shorthand method to send FairMQParts on `chan` at index `i`
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    inline int64_t Send(FairMQParts& parts, const std::string& chan, const int i, int sndTimeoutInMs) const
    {
        return fChannels.at(chan).at(i).Send(parts.fParts, sndTimeoutInMs);
    }

    /// Shorthand method to receive FairMQParts on `chan` at index `i`
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    inline int64_t Receive(FairMQParts& parts, const std::string& chan, const int i, int rcvTimeoutInMs) const
    {
        return fChannels.at(chan).at(i).Receive(parts.fParts, rcvTimeoutInMs);
    }

    /// Shorthand method to send FairMQParts on `chan` at index `i` without blocking
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    inline int64_t SendAsync(FairMQParts& parts, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).SendAsync(parts.fParts);
    }

    /// Shorthand method to receive FairMQParts on `chan` at index `i` without blocking
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    inline int64_t ReceiveAsync(FairMQParts& parts, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).ReceiveAsync(parts.fParts);
    }

    /// @brief Getter for default transport factory
    auto Transport() const -> const FairMQTransportFactory*
    {
        return fTransports.cbegin()->second.get();
    }

    template<typename... Args>
    inline FairMQMessagePtr NewMessage(Args&&... args) const
    {
        return Transport()->CreateMessage(std::forward<Args>(args)...);
    }

    template<typename... Args>
    inline FairMQMessagePtr NewMessageFor(const std::string& channel, int index, Args&&... args) const
    {
        return fChannels.at(channel).at(index).Transport()->CreateMessage(std::forward<Args>(args)...);
    }

    template<typename T>
    inline FairMQMessagePtr NewStaticMessage(const T& data) const
    {
        return Transport()->NewStaticMessage(data);
    }

    template<typename T>
    inline FairMQMessagePtr NewStaticMessageFor(const std::string& channel, int index, const T& data) const
    {
        return fChannels.at(channel).at(index).NewStaticMessage(data);
    }

    template<typename T>
    inline FairMQMessagePtr NewSimpleMessage(const T& data) const
    {
        return Transport()->NewSimpleMessage(data);
    }

    template<typename T>
    inline FairMQMessagePtr NewSimpleMessageFor(const std::string& channel, int index, const T& data) const
    {
        return fChannels.at(channel).at(index).NewSimpleMessage(data);
    }

    template<typename ...Ts>
    FairMQPollerPtr NewPoller(const Ts&... inputs)
    {
        std::vector<std::string> chans{inputs...};

        // if more than one channel provided, check compatibility
        if (chans.size() > 1)
        {
            FairMQ::Transport type = fChannels.at(chans.at(0)).at(0).Transport()->GetType();

            for (int i = 1; i < chans.size(); ++i)
            {
                if (type != fChannels.at(chans.at(i)).at(0).Transport()->GetType())
                {
                    LOG(ERROR) << "FairMQDevice::NewPoller() failed: different transports within same poller are not yet supported. Going to ERROR state.";
                    ChangeState(ERROR_FOUND);
                }
            }
        }

        return fChannels.at(chans.at(0)).at(0).Transport()->CreatePoller(fChannels, chans);
    }

    FairMQPollerPtr NewPoller(const std::vector<const FairMQChannel*>& channels)
    {
        // if more than one channel provided, check compatibility
        if (channels.size() > 1)
        {
            FairMQ::Transport type = channels.at(0)->Transport()->GetType();

            for (int i = 1; i < channels.size(); ++i)
            {
                if (type != channels.at(i)->Transport()->GetType())
                {
                    LOG(ERROR) << "FairMQDevice::NewPoller() failed: different transports within same poller are not yet supported. Going to ERROR state.";
                    ChangeState(ERROR_FOUND);
                }
            }
        }

        return channels.at(0)->Transport()->CreatePoller(channels);
    }

    /// Waits for the first initialization run to finish
    void WaitForInitialValidation();

    /// Starts interactive (console) loop for controlling the device
    /// Works only when running in a terminal. Running in background would exit, because no interactive input (std::cin) is possible.
    void InteractiveStateLoop();
    /// Prints the available commands of the InteractiveStateLoop()
    inline void PrintInteractiveStateLoopHelp()
    {
        LOG(INFO) << "Use keys to control the state machine:";
        LOG(INFO) << "[h] help, [p] pause, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device";
    }

    /// Set Device properties stored as strings
    /// @param key      Property key
    /// @param value    Property value
    virtual void SetProperty(const int key, const std::string& value);
    /// Get Device properties stored as strings
    /// @param key      Property key
    /// @param default_ not used
    /// @return         Property value
    virtual std::string GetProperty(const int key, const std::string& default_ = "");
    /// Set Device properties stored as integers
    /// @param key      Property key
    /// @param value    Property value
    virtual void SetProperty(const int key, const int value);
    /// Get Device properties stored as integers
    /// @param key      Property key
    /// @param default_ not used
    /// @return         Property value
    virtual int GetProperty(const int key, const int default_ = 0);

    /// Get property description for a given property name
    /// @param  key     Property name/key
    /// @return String  with the property description
    virtual std::string GetPropertyDescription(const int key);
    /// Print all properties of this and the parent class to LOG(INFO)
    virtual void ListProperties();

    /// Adds a transport to the device if it doesn't exist
    /// @param transport  Transport string ("zeromq"/"nanomsg"/"shmem")
    std::shared_ptr<FairMQTransportFactory> AddTransport(const std::string& transport);
    /// Sets the default transport for the device
    /// @param transport  Transport string ("zeromq"/"nanomsg"/"shmem")
    void SetTransport(const std::string& transport = "zeromq");

    /// Creates stand-alone transport factory
    /// @param transport  Transport string ("zeromq"/"nanomsg"/"shmem")
    static std::unique_ptr<FairMQTransportFactory> MakeTransport(const std::string& transport) __attribute__((deprecated("Use 'static auto FairMQTransportFactory::CreateTransportFactory() -> std::shared_ptr<FairMQTransportFactory>' from <FairMQTransportFactory.h> instead.")));

    void SetConfig(FairMQProgOptions& config);
    const FairMQProgOptions* GetConfig() const
    {
        return fConfig;
    }

    /// Implements the sort algorithm used in SortChannel()
    /// @param lhs Right hand side value for comparison
    /// @param rhs Left hand side value for comparison
    static bool SortSocketsByAddress(const FairMQChannel &lhs, const FairMQChannel &rhs);

    template<class T>
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


    template<class T>
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

    bool Terminated();

    const FairMQChannel& GetChannel(const std::string& channelName, const int index = 0) const;

    virtual void RegisterChannelEndpoints() {}

    bool RegisterChannelEndpoint(const std::string& channelName, uint16_t minNumSubChannels = 1, uint16_t maxNumSubChannels = 1)
    {
        bool ok = fChannelRegistry.insert(std::make_pair(channelName, std::make_pair(minNumSubChannels, maxNumSubChannels))).second;
        if (!ok)
        {
            LOG(WARN) << "Registering channel: name already registered: \"" << channelName << "\"";
        }
        return ok;
    }

    void PrintRegisteredChannels()
    {
        if (fChannelRegistry.size() < 1)
        {
            std::cout << "no channels registered." << std::endl;
        }
        else
        {
            for (const auto& c : fChannelRegistry)
            {
                std::cout << c.first << ":" << c.second.first << ":" << c.second.second << std::endl;
            }
        }
    }

    void SetId(const std::string& id) { fId = id; }
    std::string GetId() { return fId; }

    void SetNumIoThreads(int numIoThreads) { fNumIoThreads = numIoThreads; }
    int GetNumIoThreads() { return fNumIoThreads; }

    void SetPortRangeMin(int portRangeMin) { fPortRangeMin = portRangeMin; }
    int GetPortRangeMin() { return fPortRangeMin; }

    void SetPortRangeMax(int portRangeMax) { fPortRangeMax = portRangeMax; }
    int GetPortRangeMax() { return fPortRangeMax; }

    void SetNetworkInterface(const std::string& networkInterface) { fNetworkInterface = networkInterface; }
    std::string GetNetworkInterface() { return fNetworkInterface; }

    void SetDefaultTransport(const std::string& defaultTransport) { fDefaultTransport = defaultTransport; }
    std::string GetDefaultTransport() { return fDefaultTransport; }

    void SetInitializationTimeoutInS(int initializationTimeoutInS) { fInitializationTimeoutInS = initializationTimeoutInS; }
    int GetInitializationTimeoutInS() { return fInitializationTimeoutInS; }

  protected:
    std::shared_ptr<FairMQTransportFactory> fTransportFactory; ///< Transport factory
    std::unordered_map<FairMQ::Transport, std::shared_ptr<FairMQTransportFactory>> fTransports; ///< Container for transports

  public:
    std::unordered_map<std::string, std::vector<FairMQChannel>> fChannels; ///< Device channels
    FairMQProgOptions* fConfig; ///< Program options configuration

  protected:
    std::string fId; ///< Device ID

    int fNumIoThreads; ///< Number of ZeroMQ I/O threads

    /// Additional user initialization (can be overloaded in child classes). Prefer to use InitTask().
    virtual void Init();

    /// Task initialization (can be overloaded in child classes)
    virtual void InitTask();

    /// Runs the device (to be overloaded in child classes)
    virtual void Run();

    /// Called in the RUNNING state once before executing the Run()/ConditionalRun() method
    virtual void PreRun();

    /// Called during RUNNING state repeatedly until it returns false or device state changes
    virtual bool ConditionalRun();

    /// Called in the RUNNING state once after executing the Run()/ConditionalRun() method
    virtual void PostRun();

    /// Handles the PAUSE state
    virtual void Pause();

    /// Resets the user task (to be overloaded in child classes)
    virtual void ResetTask();

    /// Resets the device (can be overloaded in child classes)
    virtual void Reset();

  private:
    // condition variable to notify parent thread about end of initial validation.
    bool fInitialValidationFinished;
    std::condition_variable fInitialValidationCondition;
    std::mutex fInitialValidationMutex;

    int fPortRangeMin; ///< Minimum value for the port range (if dynamic)
    int fPortRangeMax; ///< Maximum value for the port range (if dynamic)

    std::string fNetworkInterface; ///< Network interface to use for dynamic binding
    std::string fDefaultTransport; ///< Default transport for the device

    int fInitializationTimeoutInS; ///< Timeout for the initialization (in seconds)

    /// Handles the initialization and the Init() method
    void InitWrapper();
    /// Handles the InitTask() method
    void InitTaskWrapper();
    /// Handles the Run() method
    void RunWrapper();
    /// Handles the Pause() method
    void PauseWrapper();
    /// Handles the ResetTask() method
    void ResetTaskWrapper();
    /// Handles the Reset() method
    void ResetWrapper();

    /// Unblocks blocking channel send/receive calls
    void Unblock();

    /// Shuts down the transports and the device
    void Exit();

    /// Attach (bind/connect) channels in the list
    void AttachChannels(std::list<FairMQChannel*>& chans);

    /// Sets up and connects/binds a socket to an endpoint
    /// return a string with the actual endpoint if it happens
    /// to stray from default.
    bool ConnectEndpoint(FairMQSocket& socket, std::string& endpoint);
    bool BindEndpoint(FairMQSocket& socket, std::string& endpoint);
    /// Attaches the channel to all listed endpoints
    /// the list is comma separated; the default method (bind/connect) is used.
    /// to override default: prepend "@" to bind, "+" or ">" to connect endpoint.
    bool AttachChannel(FairMQChannel& ch);

    void HandleSingleChannelInput();
    void HandleMultipleChannelInput();
    void HandleMultipleTransportInput();
    void PollForTransport(const FairMQTransportFactory* factory, const std::vector<std::string>& channelKeys);

    bool HandleMsgInput(const std::string& chName, const InputMsgCallback& callback, int i) const;
    bool HandleMultipartInput(const std::string& chName, const InputMultipartCallback& callback, int i) const;

    void CreateOwnConfig();

    /// Signal handler
    void SignalHandler(int signal);
    bool fCatchingSignals;
    bool fTerminationRequested;
    // Interactive state loop helper
    std::atomic<bool> fInteractiveRunning;

    bool fDataCallbacks;
    std::unordered_map<FairMQ::Transport, FairMQSocketPtr> fDeviceCmdSockets; ///< Sockets used for the internal unblocking mechanism
    std::unordered_map<std::string, InputMsgCallback> fMsgInputs;
    std::unordered_map<std::string, InputMultipartCallback> fMultipartInputs;
    std::unordered_map<FairMQ::Transport, std::vector<std::string>> fMultitransportInputs;
    std::unordered_map<std::string, std::pair<uint16_t, uint16_t>> fChannelRegistry;
    std::vector<std::string> fInputChannelKeys;
    std::mutex fMultitransportMutex;
    std::atomic<bool> fMultitransportProceed;

    bool fExternalConfig;
};

#endif /* FAIRMQDEVICE_H_ */

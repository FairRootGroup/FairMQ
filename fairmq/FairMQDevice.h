/********************************************************************************
 * Copyright (C) 2012-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQDEVICE_H_
#define FAIRMQDEVICE_H_

#include <FairMQStateMachine.h>
#include <FairMQTransportFactory.h>
#include <fairmq/Transports.h>

#include <FairMQSocket.h>
#include <FairMQChannel.h>
#include <FairMQMessage.h>
#include <FairMQParts.h>
#include <FairMQUnmanagedRegion.h>
#include <FairMQLogger.h>
#include <options/FairMQProgOptions.h>

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

#include <fairmq/Tools.h>

using FairMQChannelMap = std::unordered_map<std::string, std::vector<FairMQChannel>>;

using InputMsgCallback = std::function<bool(FairMQMessagePtr&, int)>;
using InputMultipartCallback = std::function<bool(FairMQParts&, int)>;

class FairMQDevice : public FairMQStateMachine
{
    friend class FairMQChannel;

  public:
    /// Default constructor
    FairMQDevice();

    /// Constructor that sets the version
    FairMQDevice(const fair::mq::tools::Version version);
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

    int Send(FairMQMessagePtr& msg, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Send(msg);
    }

    int Receive(FairMQMessagePtr& msg, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Receive(msg);
    }

    /// Shorthand method to send `msg` on `chan` at index `i`
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    int Send(FairMQMessagePtr& msg, const std::string& chan, const int i, int sndTimeoutInMs) const
    {
        return fChannels.at(chan).at(i).Send(msg, sndTimeoutInMs);
    }

    /// Shorthand method to receive `msg` on `chan` at index `i`
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    int Receive(FairMQMessagePtr& msg, const std::string& chan, const int i, int rcvTimeoutInMs) const
    {
        return fChannels.at(chan).at(i).Receive(msg, rcvTimeoutInMs);
    }

    /// Shorthand method to send `msg` on `chan` at index `i` without blocking
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    int SendAsync(FairMQMessagePtr& msg, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).SendAsync(msg);
    }

    /// Shorthand method to receive `msg` on `chan` at index `i` without blocking
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    int ReceiveAsync(FairMQMessagePtr& msg, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).ReceiveAsync(msg);
    }

    int64_t Send(FairMQParts& parts, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Send(parts.fParts);
    }

    int64_t Receive(FairMQParts& parts, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Receive(parts.fParts);
    }

    /// Shorthand method to send FairMQParts on `chan` at index `i`
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    int64_t Send(FairMQParts& parts, const std::string& chan, const int i, int sndTimeoutInMs) const
    {
        return fChannels.at(chan).at(i).Send(parts.fParts, sndTimeoutInMs);
    }

    /// Shorthand method to receive FairMQParts on `chan` at index `i`
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    int64_t Receive(FairMQParts& parts, const std::string& chan, const int i, int rcvTimeoutInMs) const
    {
        return fChannels.at(chan).at(i).Receive(parts.fParts, rcvTimeoutInMs);
    }

    /// Shorthand method to send FairMQParts on `chan` at index `i` without blocking
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    int64_t SendAsync(FairMQParts& parts, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).SendAsync(parts.fParts);
    }

    /// Shorthand method to receive FairMQParts on `chan` at index `i` without blocking
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    int64_t ReceiveAsync(FairMQParts& parts, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).ReceiveAsync(parts.fParts);
    }

    /// @brief Getter for default transport factory
    auto Transport() const -> const FairMQTransportFactory*
    {
        return fTransportFactory.get();
    }

    template<typename... Args>
    FairMQMessagePtr NewMessage(Args&&... args) const
    {
        return Transport()->CreateMessage(std::forward<Args>(args)...);
    }

    template<typename... Args>
    FairMQMessagePtr NewMessageFor(const std::string& channel, int index, Args&&... args) const
    {
        return fChannels.at(channel).at(index).Transport()->CreateMessage(std::forward<Args>(args)...);
    }

    template<typename T>
    FairMQMessagePtr NewStaticMessage(const T& data) const
    {
        return Transport()->NewStaticMessage(data);
    }

    template<typename T>
    FairMQMessagePtr NewStaticMessageFor(const std::string& channel, int index, const T& data) const
    {
        return fChannels.at(channel).at(index).NewStaticMessage(data);
    }

    template<typename T>
    FairMQMessagePtr NewSimpleMessage(const T& data) const
    {
        return Transport()->NewSimpleMessage(data);
    }

    template<typename T>
    FairMQMessagePtr NewSimpleMessageFor(const std::string& channel, int index, const T& data) const
    {
        return fChannels.at(channel).at(index).NewSimpleMessage(data);
    }

    FairMQUnmanagedRegionPtr NewUnmanagedRegion(const size_t size)
    {
        return Transport()->CreateUnmanagedRegion(size);
    }

    FairMQUnmanagedRegionPtr NewUnmanagedRegionFor(const std::string& channel, int index, const size_t size, FairMQRegionCallback callback = nullptr)
    {
        return fChannels.at(channel).at(index).Transport()->CreateUnmanagedRegion(size, callback);
    }

    template<typename ...Ts>
    FairMQPollerPtr NewPoller(const Ts&... inputs)
    {
        std::vector<std::string> chans{inputs...};

        // if more than one channel provided, check compatibility
        if (chans.size() > 1)
        {
            fair::mq::Transport type = fChannels.at(chans.at(0)).at(0).Transport()->GetType();

            for (unsigned int i = 1; i < chans.size(); ++i)
            {
                if (type != fChannels.at(chans.at(i)).at(0).Transport()->GetType())
                {
                    LOG(error) << "poller failed: different transports within same poller are not yet supported. Going to ERROR state.";
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
            fair::mq::Transport type = channels.at(0)->Transport()->GetType();

            for (unsigned int i = 1; i < channels.size(); ++i)
            {
                if (type != channels.at(i)->Transport()->GetType())
                {
                    LOG(error) << "poller failed: different transports within same poller are not yet supported. Going to ERROR state.";
                    ChangeState(ERROR_FOUND);
                }
            }
        }

        return channels.at(0)->Transport()->CreatePoller(channels);
    }

    /// Waits for the first initialization run to finish
    void WaitForInitialValidation();

    /// Adds a transport to the device if it doesn't exist
    /// @param transport  Transport string ("zeromq"/"nanomsg"/"shmem")
    std::shared_ptr<FairMQTransportFactory> AddTransport(const fair::mq::Transport transport);
    /// Sets the default transport for the device
    /// @param transport  Transport string ("zeromq"/"nanomsg"/"shmem")
    void SetTransport(const std::string& transport = "zeromq");

    void SetConfig(FairMQProgOptions& config);
    const FairMQProgOptions* GetConfig() const
    {
        return fConfig;
    }

    /// Implements the sort algorithm used in SortChannel()
    /// @param lhs Right hand side value for comparison
    /// @param rhs Left hand side value for comparison
    static bool SortSocketsByAddress(const FairMQChannel &lhs, const FairMQChannel &rhs);

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

    const FairMQChannel& GetChannel(const std::string& channelName, const int index = 0) const;

    virtual void RegisterChannelEndpoints() {}

    bool RegisterChannelEndpoint(const std::string& channelName, uint16_t minNumSubChannels = 1, uint16_t maxNumSubChannels = 1)
    {
        bool ok = fChannelRegistry.insert(std::make_pair(channelName, std::make_pair(minNumSubChannels, maxNumSubChannels))).second;
        if (!ok)
        {
            LOG(warn) << "Registering channel: name already registered: \"" << channelName << "\"";
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

    const fair::mq::tools::Version GetVersion() const { return fVersion; }

    void SetNumIoThreads(int numIoThreads) { fNumIoThreads = numIoThreads; }
    int GetNumIoThreads() const { return fNumIoThreads; }

    void SetPortRangeMin(int portRangeMin) { fPortRangeMin = portRangeMin; }
    int GetPortRangeMin() const { return fPortRangeMin; }

    void SetPortRangeMax(int portRangeMax) { fPortRangeMax = portRangeMax; }
    int GetPortRangeMax() const { return fPortRangeMax; }

    void SetNetworkInterface(const std::string& networkInterface) { fNetworkInterface = networkInterface; }
    std::string GetNetworkInterface() const { return fNetworkInterface; }

    void SetDefaultTransport(const std::string& name) { fDefaultTransportType = fair::mq::TransportTypes.at(name); }
    std::string GetDefaultTransport() const { return fair::mq::TransportNames.at(fDefaultTransportType); }

    void SetInitializationTimeoutInS(int initializationTimeoutInS) { fInitializationTimeoutInS = initializationTimeoutInS; }
    int GetInitializationTimeoutInS() const { return fInitializationTimeoutInS; }

    void SetRawCmdLineArgs(const std::vector<std::string>& args) { fRawCmdLineArgs = args; }
    std::vector<std::string> GetRawCmdLineArgs() const { return fRawCmdLineArgs; }

  protected:
    std::shared_ptr<FairMQTransportFactory> fTransportFactory; ///< Default transport factory
    std::unordered_map<fair::mq::Transport, std::shared_ptr<FairMQTransportFactory>> fTransports; ///< Container for transports

  public:
    std::unordered_map<std::string, std::vector<FairMQChannel>> fChannels; ///< Device channels
    FairMQProgOptions* fConfig; ///< Program options configuration

  protected:
    std::string fId; ///< Device ID

    int fNumIoThreads; ///< Number of ZeroMQ I/O threads

    /// Additional user initialization (can be overloaded in child classes). Prefer to use InitTask().
    /// Executed in a worker thread
    virtual void Init();

    /// Task initialization (can be overloaded in child classes)
    /// Executed in a worker thread
    virtual void InitTask();

    /// Runs the device (to be overloaded in child classes)
    /// Executed in a worker thread
    virtual void Run();

    /// Called in the RUNNING state once before executing the Run()/ConditionalRun() method
    /// Executed in a worker thread
    virtual void PreRun();

    /// Called during RUNNING state repeatedly until it returns false or device state changes
    /// Executed in a worker thread
    virtual bool ConditionalRun();

    /// Called in the RUNNING state once after executing the Run()/ConditionalRun() method
    /// Executed in a worker thread
    virtual void PostRun();

    /// Handles the PAUSE state
    /// Executed in a worker thread
    virtual void Pause();

    /// Resets the user task (to be overloaded in child classes)
    /// Executed in a worker thread
    virtual void ResetTask();

    /// Resets the device (can be overloaded in child classes)
    /// Executed in a worker thread
    virtual void Reset();

  private:
    // condition variable to notify parent thread about end of initial validation.
    bool fInitialValidationFinished;
    std::condition_variable fInitialValidationCondition;
    std::mutex fInitialValidationMutex;

    int fPortRangeMin; ///< Minimum value for the port range (if dynamic)
    int fPortRangeMax; ///< Maximum value for the port range (if dynamic)

    std::string fNetworkInterface; ///< Network interface to use for dynamic binding
    fair::mq::Transport fDefaultTransportType; ///< Default transport for the device

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
    void AttachChannels(std::vector<FairMQChannel*>& chans);

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

    bool fDataCallbacks;
    std::unordered_map<std::string, InputMsgCallback> fMsgInputs;
    std::unordered_map<std::string, InputMultipartCallback> fMultipartInputs;
    std::unordered_map<fair::mq::Transport, std::vector<std::string>> fMultitransportInputs;
    std::unordered_map<std::string, std::pair<uint16_t, uint16_t>> fChannelRegistry;
    std::vector<std::string> fInputChannelKeys;
    std::mutex fMultitransportMutex;
    std::atomic<bool> fMultitransportProceed;

    bool fExternalConfig;

    const fair::mq::tools::Version fVersion;
    float fRate; ///< Rate limiting for ConditionalRun
    size_t fLastTime; ///< Rate limiting for ConditionalRun
    std::vector<std::string> fRawCmdLineArgs;
};

#endif /* FAIRMQDEVICE_H_ */

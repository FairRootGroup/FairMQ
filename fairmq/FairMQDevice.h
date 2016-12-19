/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQDevice.h
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQDEVICE_H_
#define FAIRMQDEVICE_H_

#include <vector>
#include <memory> // unique_ptr
#include <string>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <assert.h> // static_assert
#include <type_traits> // is_trivially_copyable

#include <mutex>
#include <condition_variable>

#include "FairMQConfigurable.h"
#include "FairMQStateMachine.h"
#include "FairMQTransportFactory.h"

#include "FairMQSocket.h"
#include "FairMQChannel.h"
#include "FairMQMessage.h"
#include "FairMQParts.h"

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
        MaxInitializationAttempts, ///< Timeout for the initialization
        NumIoThreads, ///< Number of ZeroMQ I/O threads
        PortRangeMin, ///< Minimum value for the port range (if dynamic)
        PortRangeMax, ///< Maximum value for the port range (if dynamic)
        LogIntervalInMs, ///< Interval for logging the socket transfer rates
        NetworkInterface, ///< Network interface to use for dynamic binding
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

    /// Shorthand method to send `msg` on `chan` at index `i`
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    inline int Send(const FairMQMessagePtr& msg, const std::string& chan, const int i = 0, int sndTimeoutInMs = -1) const
    {
        return fChannels.at(chan).at(i).Send(msg, sndTimeoutInMs);
    }

    /// Shorthand method to send `msg` on `chan` at index `i` without blocking
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    inline int SendAsync(const FairMQMessagePtr& msg, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).SendAsync(msg);
    }

    /// Shorthand method to send FairMQParts on `chan` at index `i`
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    inline int64_t Send(const FairMQParts& parts, const std::string& chan, const int i = 0, int sndTimeoutInMs = -1) const
    {
        return fChannels.at(chan).at(i).Send(parts.fParts, sndTimeoutInMs);
    }

    /// Shorthand method to send FairMQParts on `chan` at index `i` without blocking
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out.
    /// In case of errors, returns -1.
    inline int64_t SendAsync(const FairMQParts& parts, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).SendAsync(parts.fParts);
    }

    /// Shorthand method to receive `msg` on `chan` at index `i`
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    inline int Receive(const FairMQMessagePtr& msg, const std::string& chan, const int i = 0, int rcvTimeoutInMs = -1) const
    {
        return fChannels.at(chan).at(i).Receive(msg, rcvTimeoutInMs);
    }

    /// Shorthand method to receive `msg` on `chan` at index `i` without blocking
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    inline int ReceiveAsync(const FairMQMessagePtr& msg, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).ReceiveAsync(msg);
    }

    /// Shorthand method to receive FairMQParts on `chan` at index `i`
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out.
    /// In case of errors, returns -1.
    inline int64_t Receive(FairMQParts& parts, const std::string& chan, const int i = 0, int rcvTimeoutInMs = -1) const
    {
        return fChannels.at(chan).at(i).Receive(parts.fParts, rcvTimeoutInMs);
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

    /// @brief Create empty FairMQMessage
    /// @return pointer to FairMQMessage
    inline FairMQMessagePtr NewMessage() const
    {
        return fTransportFactory->CreateMessage();
    }

    /// @brief Create new FairMQMessage of specified size
    /// @param size message size
    /// @return pointer to FairMQMessage
    inline FairMQMessagePtr NewMessage(int size) const
    {
        return fTransportFactory->CreateMessage(size);
    }

    template<typename T>
    static void FairMQSimpleMsgCleanup(void* /*data*/, void* hint)
    {
        delete static_cast<T*>(hint);
    }

    static void FairMQNoCleanup(void* /*data*/, void* /*hint*/)
    {
    }

    /// @brief Create new FairMQMessage with user provided buffer and size
    /// @param data pointer to user provided buffer
    /// @param size size of the user provided buffer
    /// @param ffn optional callback, called when the message is transfered (and can be deleted)
    /// @param hint optional helper pointer that can be used in the callback
    /// @return pointer to FairMQMessage
    inline FairMQMessagePtr NewMessage(void* data, int size, fairmq_free_fn* ffn, void* hint = nullptr) const
    {
        return fTransportFactory->CreateMessage(data, size, ffn, hint);
    }

    template<typename T>
    inline FairMQMessagePtr NewStaticMessage(const T& data) const
    {
        return fTransportFactory->CreateMessage(data, sizeof(T), FairMQNoCleanup, nullptr);
    }

    inline FairMQMessagePtr NewStaticMessage(const std::string& str) const
    {
        return fTransportFactory->CreateMessage(const_cast<char*>(str.c_str()), str.length(), FairMQNoCleanup, nullptr);
    }

    template<typename T>
    inline FairMQMessagePtr NewSimpleMessage(const T& data) const
    {
        // todo: is_trivially_copyable not available on gcc < 5, workaround?
        // static_assert(std::is_trivially_copyable<T>::value, "The argument type for NewSimpleMessage has to be trivially copyable!");
        T* dataCopy = new T(data);
        return fTransportFactory->CreateMessage(dataCopy, sizeof(T), FairMQSimpleMsgCleanup<T>, dataCopy);
    }

    template<std::size_t N>
    inline FairMQMessagePtr NewSimpleMessage(const char(&data)[N]) const
    {
        std::string* msgStr = new std::string(data);
        return fTransportFactory->CreateMessage(const_cast<char*>(msgStr->c_str()), msgStr->length(), FairMQSimpleMsgCleanup<std::string>, msgStr);
    }

    inline FairMQMessagePtr NewSimpleMessage(const std::string& str) const
    {
        std::string* msgStr = new std::string(str);
        return fTransportFactory->CreateMessage(const_cast<char*>(msgStr->c_str()), msgStr->length(), FairMQSimpleMsgCleanup<std::string>, msgStr);
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

    /// Configures the device with a transport factory (DEPRECATED)
    /// @param factory  Pointer to the transport factory object
    void SetTransport(FairMQTransportFactory* factory);
    /// Configures the device with a transport factory
    /// @param transport  Transport string ("zeromq"/"nanomsg")
    void SetTransport(const std::string& transport = "zeromq");

    void SetConfig(FairMQProgOptions& config);

    /// Implements the sort algorithm used in SortChannel()
    /// @param lhs Right hand side value for comparison
    /// @param rhs Left hand side value for comparison
    static bool SortSocketsByAddress(const FairMQChannel &lhs, const FairMQChannel &rhs);

    std::unordered_map<std::string, std::vector<FairMQChannel>> fChannels; ///< Device channels
    FairMQProgOptions* fConfig; ///< Program options configuration

    template<class T>
    void OnData(const std::string& channelName, bool (T::* memberFunction)(FairMQMessagePtr& msg, int index))
    {
        fDataCallbacks = true;
        fMsgInputs.insert(std::make_pair(channelName, [this, memberFunction](FairMQMessagePtr& msg, int index)
        {
            return (static_cast<T*>(this)->*memberFunction)(msg, index);
        }));
    }

    void OnData(const std::string& channelName, InputMsgCallback);

    template<class T>
    void OnData(const std::string& channelName, bool (T::* memberFunction)(FairMQParts& parts, int index))
    {
        fDataCallbacks = true;
        fMultipartInputs.insert(std::make_pair(channelName, [this, memberFunction](FairMQParts& parts, int index)
        {
            return (static_cast<T*>(this)->*memberFunction)(parts, index);
        }));
    }

    void OnData(const std::string& channelName, InputMultipartCallback);

    bool Terminated();

  protected:
    std::string fId; ///< Device ID
    std::string fNetworkInterface; ///< Network interface to use for dynamic binding

    int fMaxInitializationAttempts; ///< Timeout for the initialization

    int fNumIoThreads; ///< Number of ZeroMQ I/O threads

    int fPortRangeMin; ///< Minimum value for the port range (if dynamic)
    int fPortRangeMax; ///< Maximum value for the port range (if dynamic)

    int fLogIntervalInMs; ///< Interval for logging the socket transfer rates

    FairMQSocketPtr fCmdSocket; ///< Socket used for the internal unblocking mechanism

    std::shared_ptr<FairMQTransportFactory> fTransportFactory; ///< Transport factory

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

    /// Handles the initialization and the Init() method
    void InitWrapper();
    /// Handles the InitTask() method
    void InitTaskWrapper();
    /// Handles the Run() method
    void RunWrapper();
    /// Handles the ResetTask() method
    void ResetTaskWrapper();
    /// Handles the Reset() method
    void ResetWrapper();
    /// Shuts down the device (closses socket connections)
    void Shutdown();

    /// Terminates the transport interface
    void Terminate();
    /// Unblocks blocking channel send/receive calls
    void Unblock();

    /// Binds channel in the list
    void BindChannels(std::list<FairMQChannel*>& chans);
    /// Connects channel in the list
    void ConnectChannels(std::list<FairMQChannel*>& chans);
    /// Binds a single channel (used in InitWrapper)
    bool BindChannel(FairMQChannel& ch);
    /// Connects a single channel (used in InitWrapper)
    bool ConnectChannel(FairMQChannel& ch);

    /// Sets up and connects/binds a socket to an endpoint
    /// return a string with the actual endpoint if it happens
    //to stray from default.
    bool ConnectEndpoint(FairMQSocket& socket, std::string& endpoint);
    bool BindEndpoint(FairMQSocket& socket, std::string& endpoint);
    /// Attaches the channel to all listed endpoints
    /// the list is comma separated; the default method (bind/connect) is used.
    /// to override default: prepend "@" to bind, "+" or ">" to connect endpoint.
    bool AttachChannel(FairMQChannel& ch);

    /// Signal handler
    void SignalHandler(int signal);
    bool fCatchingSignals;
    bool fTerminationRequested;
    // Interactive state loop helper
    std::atomic<bool> fInteractiveRunning;

    bool fDataCallbacks;
    std::unordered_map<std::string, InputMsgCallback> fMsgInputs;
    std::unordered_map<std::string, InputMultipartCallback> fMultipartInputs;
};

#endif /* FAIRMQDEVICE_H_ */

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

#include "FairMQConfigurable.h"
#include "FairMQStateMachine.h"
#include "FairMQTransportFactory.h"
#include "FairMQSocket.h"
#include "FairMQChannel.h"
#include "FairMQMessage.h"
#include "FairMQParts.h"

class FairMQDevice : public FairMQStateMachine, public FairMQConfigurable
{
    friend class FairMQChannel;

  public:
    enum
    {
        Id = FairMQConfigurable::Last, ///< Device ID
        MaxInitializationTime, ///< Timeout for the initialization
        NumIoThreads, ///< Number of ZeroMQ I/O threads
        PortRangeMin, ///< Minimum value for the port range (if dynamic)
        PortRangeMax, ///< Maximum value for the port range (if dynamic)
        LogIntervalInMs, ///< Interval for logging the socket transfer rates
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

    /// Shorthand method to send `msg` on `chan` at index `i`
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out. In case of errors, returns -1.
    inline int Send(const std::unique_ptr<FairMQMessage>& msg, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Send(msg);
    }

    /// Shorthand method to receive `msg` on `chan` at index `i`
    /// @param msg message reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out. In case of errors, returns -1.
    inline int Receive(const std::unique_ptr<FairMQMessage>& msg, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Receive(msg);
    }

    /// Shorthand method to send a vector of messages on `chan` at index `i`
    /// @param msgVec message vector reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out. In case of errors, returns -1.
    inline int64_t Send(const std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Send(msgVec);
    }

    /// Shorthand method to receive a vector of messages on `chan` at index `i`
    /// @param msgVec message vector reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out. In case of errors, returns -1.
    inline int64_t Receive(std::vector<std::unique_ptr<FairMQMessage>>& msgVec, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Receive(msgVec);
    }

    /// Shorthand method to send FairMQParts on `chan` at index `i`
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been queued. -2 If queueing was not possible or timed out. In case of errors, returns -1.
    inline int64_t Send(const FairMQParts& parts, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Send(parts.fParts);
    }

    /// Shorthand method to receive FairMQParts on `chan` at index `i`
    /// @param parts parts reference
    /// @param chan channel name
    /// @param i channel index
    /// @return Number of bytes that have been received. -2 If reading from the queue was not possible or timed out. In case of errors, returns -1.
    inline int64_t Receive(FairMQParts& parts, const std::string& chan, const int i = 0) const
    {
        return fChannels.at(chan).at(i).Receive(parts.fParts);
    }

    /// @brief Create empty FairMQMessage
    /// @return pointer to FairMQMessage
    inline FairMQMessage* NewMessage() const
    {
        return fTransportFactory->CreateMessage();
    }

    /// @brief Create new FairMQMessage of specified size
    /// @param size message size
    /// @return pointer to FairMQMessage
    inline FairMQMessage* NewMessage(int size) const
    {
        return fTransportFactory->CreateMessage(size);
    }

    /// @brief Create new FairMQMessage with user provided buffer and size
    /// @param data pointer to user provided buffer
    /// @param size size of the user provided buffer
    /// @param ffn optional callback, called when the message is transfered (and can be deleted)
    /// @param hint optional helper pointer that can be used in the callback
    /// @return pointer to FairMQMessage
    inline FairMQMessage* NewMessage(void* data, int size, fairmq_free_fn* ffn = NULL, void* hint = NULL) const
    {
        return fTransportFactory->CreateMessage(data, size, ffn, hint);
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

    /// Implements the sort algorithm used in SortChannel()
    /// @param lhs Right hand side value for comparison
    /// @param rhs Left hand side value for comparison
    static bool SortSocketsByAddress(const FairMQChannel &lhs, const FairMQChannel &rhs);

    std::unordered_map<std::string, std::vector<FairMQChannel>> fChannels; ///< Device channels

  protected:
    std::string fId; ///< Device ID

    int fMaxInitializationTime; ///< Timeout for the initialization

    int fNumIoThreads; ///< Number of ZeroMQ I/O threads

    int fPortRangeMin; ///< Minimum value for the port range (if dynamic)
    int fPortRangeMax; ///< Maximum value for the port range (if dynamic)

    int fLogIntervalInMs; ///< Interval for logging the socket transfer rates

    FairMQSocket* fCmdSocket; ///< Socket used for the internal unblocking mechanism

    FairMQTransportFactory* fTransportFactory; ///< Transport factory

    /// Additional user initialization (can be overloaded in child classes). Prefer to use InitTask().
    virtual void Init();

    /// Task initialization (can be overloaded in child classes)
    virtual void InitTask();

    /// Runs the device (to be overloaded in child classes)
    virtual void Run();

    /// Handles the PAUSE state
    virtual void Pause();

    /// Resets the user task (to be overloaded in child classes)
    virtual void ResetTask();

    /// Resets the device (can be overloaded in child classes)
    virtual void Reset();

  private:
    // condition variable to notify parent thread about end of initial validation.
    bool fInitialValidationFinished;
    boost::condition_variable fInitialValidationCondition;
    boost::mutex fInitialValidationMutex;

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

    /// Initializes a single channel (used in InitWrapper)
    bool InitChannel(FairMQChannel&);

    /// Signal handler
    void SignalHandler(int signal);
    bool fCatchingSignals;
};

#endif /* FAIRMQDEVICE_H_ */

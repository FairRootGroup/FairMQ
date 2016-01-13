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

    /// Waits for the first initialization run to finish
    void WaitForInitialValidation();

    /// Starts interactive (console) loop for controlling the device
    /// Works only when running in a terminal. Running in background would exit, because no interactive input (std::cin) is possible.
    void InteractiveStateLoop();
    /// Prints the available commands of the InteractiveStateLoop()
    void PrintInteractiveStateLoopHelp();

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

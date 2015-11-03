/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQDevice.cxx
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#include <list>
#include <algorithm> // std::sort()
#include <csignal> // catching system signals
#include <cstdlib>

#include <termios.h> // for the InteractiveStateLoop

#include <boost/thread.hpp>
#include <boost/random/mersenne_twister.hpp> // for choosing random port in range
#include <boost/random/uniform_int_distribution.hpp> // for choosing random port in range

#include "FairMQSocket.h"
#include "FairMQDevice.h"
#include "FairMQLogger.h"

using namespace std;

// boost::function and a wrapper to catch the signals
boost::function<void(int)> sigHandler;
static void CallSignalHandler(int signal)
{
    sigHandler(signal);
}

FairMQDevice::FairMQDevice()
    : fChannels()
    , fId()
    , fMaxInitializationTime(120)
    , fNumIoThreads(1)
    , fPortRangeMin(22000)
    , fPortRangeMax(32000)
    , fLogIntervalInMs(1000)
    , fCmdSocket(nullptr)
    , fTransportFactory(nullptr)
    , fInitialValidationFinished(false)
    , fInitialValidationCondition()
    , fInitialValidationMutex()
    , fCatchingSignals(false)
{
}

void FairMQDevice::CatchSignals()
{
    if (!fCatchingSignals)
    {
        sigHandler = bind1st(mem_fun(&FairMQDevice::SignalHandler), this);
        std::signal(SIGINT, CallSignalHandler);
        std::signal(SIGTERM, CallSignalHandler);
        fCatchingSignals = true;
    }
}

void FairMQDevice::SignalHandler(int signal)
{
    LOG(INFO) << "Caught signal " << signal;

    fState = EXITING;
    Unblock();
    fStateThread.interrupt();
    fStateThread.join();

    fTerminateStateThread = boost::thread(boost::bind(&FairMQDevice::Terminate, this));
    Shutdown();
    fTerminateStateThread.join();

    LOG(INFO) << "Exiting.";
    stop();
    // std::abort();
    exit(EXIT_FAILURE);
}

void FairMQDevice::InitWrapper()
{
    if (!fCmdSocket)
    {
        fCmdSocket = fTransportFactory->CreateSocket("pub", "device-commands", fNumIoThreads);
        fCmdSocket->Bind("inproc://commands");
    }

    // List to store the uninitialized channels.
    list<FairMQChannel*> uninitializedChannels;
    for (auto mi = fChannels.begin(); mi != fChannels.end(); ++mi)
    {
        for (auto vi = (mi->second).begin(); vi != (mi->second).end(); ++vi)
        {
            // set channel name: name + vector index
            stringstream ss;
            ss << mi->first << "[" << vi - (mi->second).begin() << "]";
            vi->fChannelName = ss.str();
            // fill the uninitialized list
            uninitializedChannels.push_back(&(*vi));
        }
    }

    // go over the list of channels until all are initialized (and removed from the uninitialized list)
    int numAttempts = 0;
    int maxAttempts = fMaxInitializationTime;
    do
    {
        auto itr = uninitializedChannels.begin();

        while (itr != uninitializedChannels.end())
        {
            if ((*itr)->ValidateChannel())
            {
                if (InitChannel(*(*itr)))
                {
                    (*itr)->InitCommandInterface(fTransportFactory, fNumIoThreads);
                    uninitializedChannels.erase(itr++);
                }
                else
                {
                    LOG(ERROR) << "failed to initialize channel " << (*itr)->fChannelName;
                    ++itr;
                }
            }
            else
            {
                ++itr;
            }
        }

        // notify parent thread about completion of first validation.
        boost::lock_guard<boost::mutex> lock(fInitialValidationMutex);
        fInitialValidationFinished = true;
        fInitialValidationCondition.notify_one();

        ++numAttempts;
        if (numAttempts > maxAttempts)
        {
            LOG(ERROR) << "could not initialize all channels after " << maxAttempts << " attempts";
            // TODO: goto ERROR state;
            exit(EXIT_FAILURE);
        }

        if (numAttempts != 0)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        }
    } while (!uninitializedChannels.empty());

    Init();

    ChangeState(internal_DEVICE_READY);

    // notify parent thread about end of processing.
    boost::lock_guard<boost::mutex> lock(fStateMutex);
    fStateFinished = true;
    fStateCondition.notify_one();
}

void FairMQDevice::WaitForInitialValidation()
{
    boost::unique_lock<boost::mutex> lock(fInitialValidationMutex);
    while (!fInitialValidationFinished)
    {
        fInitialValidationCondition.wait(lock);
    }
}

void FairMQDevice::Init()
{
}

bool FairMQDevice::InitChannel(FairMQChannel& ch)
{
    LOG(DEBUG) << "Initializing channel " << ch.fChannelName << " (" << ch.fType << ")";
    // initialize the socket
    ch.fSocket = fTransportFactory->CreateSocket(ch.fType, ch.fChannelName, fNumIoThreads);
    // set high water marks
    ch.fSocket->SetOption("snd-hwm", &(ch.fSndBufSize), sizeof(ch.fSndBufSize));
    ch.fSocket->SetOption("rcv-hwm", &(ch.fRcvBufSize), sizeof(ch.fRcvBufSize));

    // TODO: make it work with ipc

    if (ch.fMethod == "bind")
    {
        // number of attempts when choosing a random port
        int maxAttempts = 1000;
        int numAttempts = 0;

        // initialize random generator
        boost::random::mt19937 gen(getpid());
        boost::random::uniform_int_distribution<> randomPort(fPortRangeMin, fPortRangeMax);

        LOG(DEBUG) << "Binding channel " << ch.fChannelName << " on " << ch.fAddress;

        // try to bind to the saved port. In case of failure, try random one.
        if (!ch.fSocket->Bind(ch.fAddress))
        {
            LOG(DEBUG) << "Could not bind to configured port, trying random port in range " << fPortRangeMin << "-" << fPortRangeMax;
            do {
                ++numAttempts;

                if (numAttempts > maxAttempts)
                {
                    LOG(ERROR) << "could not bind to any port in the given range after " << maxAttempts << " attempts";
                    return false;
                }

                size_t pos = ch.fAddress.rfind(":");
                stringstream newPort;
                newPort << (int)randomPort(gen);
                ch.fAddress = ch.fAddress.substr(0, pos + 1) + newPort.str();

                LOG(DEBUG) << "Binding channel " << ch.fChannelName << " on " << ch.fAddress;
            } while (!ch.fSocket->Bind(ch.fAddress));
        }
    }
    else
    {
        LOG(DEBUG) << "Connecting channel " << ch.fChannelName << " to " << ch.fAddress;
        ch.fSocket->Connect(ch.fAddress);
    }

    return true;
}

void FairMQDevice::InitTaskWrapper()
{
    InitTask();

    ChangeState(internal_READY);

    // notify parent thread about end of processing.
    boost::lock_guard<boost::mutex> lock(fStateMutex);
    fStateFinished = true;
    fStateCondition.notify_one();
}

void FairMQDevice::InitTask()
{
}

bool FairMQDevice::SortSocketsByAddress(const FairMQChannel &lhs, const FairMQChannel &rhs)
{
    return lhs.fAddress < rhs.fAddress;
}

void FairMQDevice::SortChannel(const string& name, const bool reindex)
{
    if (fChannels.find(name) != fChannels.end())
    {
        sort(fChannels.at(name).begin(), fChannels.at(name).end(), SortSocketsByAddress);

        if (reindex)
        {
            for (auto vi = fChannels.at(name).begin(); vi != fChannels.at(name).end(); ++vi)
            {
                // set channel name: name + vector index
                stringstream ss;
                ss << name << "[" << vi - fChannels.at(name).begin() << "]";
                vi->fChannelName = ss.str();
            }
        }
    }
    else
    {
        LOG(ERROR) << "Sorting failed: no channel with the name \"" << name << "\".";
    }
}

void FairMQDevice::PrintChannel(const string& name)
{
    if (fChannels.find(name) != fChannels.end())
    {
        for (auto vi = fChannels[name].begin(); vi != fChannels[name].end(); ++vi)
        {
            LOG(INFO) << vi->fChannelName << ": "
                      << vi->fType << " | "
                      << vi->fMethod << " | "
                      << vi->fAddress << " | "
                      << vi->fSndBufSize << " | "
                      << vi->fRcvBufSize << " | "
                      << vi->fRateLogging;
        }
    }
    else
    {
        LOG(ERROR) << "Printing failed: no channel with the name \"" << name << "\".";
    }
}

void FairMQDevice::RunWrapper()
{
    LOG(INFO) << "DEVICE: Running...";

    boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

    Run();

    try
    {
        rateLogger.interrupt();
        rateLogger.join();
    }
    catch(boost::thread_resource_error& e)
    {
        LOG(ERROR) << e.what();
        exit(EXIT_FAILURE);
    }

    if (CheckCurrentState(RUNNING))
    {
        ChangeState(internal_READY);
    }

    // notify parent thread about end of processing.
    boost::lock_guard<boost::mutex> lock(fStateMutex);
    fStateFinished = true;
    fStateCondition.notify_one();
}

void FairMQDevice::Run()
{
}

void FairMQDevice::Pause()
{
    while (true)
    {
        try
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(500));
            LOG(DEBUG) << "paused...";
        }
        catch (boost::thread_interrupted&)
        {
            LOG(INFO) << "FairMQDevice::Pause() interrupted";
            break;
        }
    }
}

// Method for setting properties represented as a string.
void FairMQDevice::SetProperty(const int key, const string& value)
{
    switch (key)
    {
        case Id:
            fId = value;
            break;
        default:
            FairMQConfigurable::SetProperty(key, value);
            break;
    }
}

// Method for setting properties represented as an integer.
void FairMQDevice::SetProperty(const int key, const int value)
{
    switch (key)
    {
        case NumIoThreads:
            fNumIoThreads = value;
            break;
        case MaxInitializationTime:
            fMaxInitializationTime = value;
            break;
        case PortRangeMin:
            fPortRangeMin = value;
            break;
        case PortRangeMax:
            fPortRangeMax = value;
            break;
        case LogIntervalInMs:
            fLogIntervalInMs = value;
            break;
        default:
            FairMQConfigurable::SetProperty(key, value);
            break;
    }
}

// Method for getting properties represented as an string.
string FairMQDevice::GetProperty(const int key, const string& default_ /*= ""*/)
{
    switch (key)
    {
        case Id:
            return fId;
        default:
            return FairMQConfigurable::GetProperty(key, default_);
    }
}

string FairMQDevice::GetPropertyDescription(const int key)
{
    switch (key)
    {
        case Id:
            return "Id: Device ID";
        case NumIoThreads:
            return "NumIoThreads: Number of I/O Threads (size of the 0MQ thread pool to handle I/O operations. If your application is using only the inproc transport for messaging you may set this to zero, otherwise set it to at least one.)";
        case MaxInitializationTime:
            return "MaxInitializationTime: Timeout for retrying validation and initialization of the channels.";
        case PortRangeMin:
            return "PortRangeMin: Minumum value for the port range (when binding to dynamic port).";
        case PortRangeMax:
            return "PortRangeMax: Maximum value for the port range (when binding to dynamic port).";
        case LogIntervalInMs:
            return "LogIntervalInMs: Time between socket rates logging outputs.";
        default:
            return FairMQConfigurable::GetPropertyDescription(key);
    }
}

void FairMQDevice::ListProperties()
{
    LOG(INFO) << "Properties of FairMQDevice:";
    for (int p = FairMQConfigurable::Last; p < FairMQDevice::Last; ++p)
    {
        LOG(INFO) << " " << GetPropertyDescription(p);
    }
    LOG(INFO) << "---------------------------";
}

// Method for getting properties represented as an integer.
int FairMQDevice::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        case NumIoThreads:
            return fNumIoThreads;
        case MaxInitializationTime:
            return fMaxInitializationTime;
        case PortRangeMin:
            return fPortRangeMin;
        case PortRangeMax:
            return fPortRangeMax;
        case LogIntervalInMs:
            return fLogIntervalInMs;
        default:
            return FairMQConfigurable::GetProperty(key, default_);
    }
}

void FairMQDevice::SetTransport(FairMQTransportFactory* factory)
{
    fTransportFactory = factory;
}

void FairMQDevice::LogSocketRates()
{
    timestamp_t t0;
    timestamp_t t1;

    timestamp_t msSinceLastLog;

    int numFilteredSockets = 0;
    vector<FairMQSocket*> filteredSockets;
    vector<string> filteredChannelNames;

    // iterate over the channels map
    for (auto mi = fChannels.begin(); mi != fChannels.end(); ++mi)
    {
        // iterate over the channels vector
        for (auto vi = (mi->second).begin(); vi != (mi->second).end(); ++vi)
        {
            if (vi->fRateLogging == 1)
            {
                filteredSockets.push_back(vi->fSocket);
                stringstream ss;
                ss << mi->first << "[" << vi - (mi->second).begin() << "]";
                filteredChannelNames.push_back(ss.str());
                ++numFilteredSockets;
            }
        }
    }

    vector<unsigned long> bytesIn(numFilteredSockets);
    vector<unsigned long> msgIn(numFilteredSockets);
    vector<unsigned long> bytesOut(numFilteredSockets);
    vector<unsigned long> msgOut(numFilteredSockets);

    vector<unsigned long> bytesInNew(numFilteredSockets);
    vector<unsigned long> msgInNew(numFilteredSockets);
    vector<unsigned long> bytesOutNew(numFilteredSockets);
    vector<unsigned long> msgOutNew(numFilteredSockets);

    vector<double> mbPerSecIn(numFilteredSockets);
    vector<double> msgPerSecIn(numFilteredSockets);
    vector<double> mbPerSecOut(numFilteredSockets);
    vector<double> msgPerSecOut(numFilteredSockets);

    int i = 0;
    for (auto itr = filteredSockets.begin(); itr != filteredSockets.end(); ++itr)
    {
        bytesIn.at(i) = (*itr)->GetBytesRx();
        bytesOut.at(i) = (*itr)->GetBytesTx();
        msgIn.at(i) = (*itr)->GetMessagesRx();
        msgOut.at(i) = (*itr)->GetMessagesTx();
        ++i;
    }

    t0 = get_timestamp();

    while (true)
    {
        try
        {
            t1 = get_timestamp();

            msSinceLastLog = (t1 - t0) / 1000.0L;

            i = 0;

            for (auto itr = filteredSockets.begin(); itr != filteredSockets.end(); itr++)
            {
                bytesInNew.at(i) = (*itr)->GetBytesRx();
                mbPerSecIn.at(i) = ((double)(bytesInNew.at(i) - bytesIn.at(i)) / (1024. * 1024.)) / (double)msSinceLastLog * 1000.;
                bytesIn.at(i) = bytesInNew.at(i);

                msgInNew.at(i) = (*itr)->GetMessagesRx();
                msgPerSecIn.at(i) = (double)(msgInNew.at(i) - msgIn.at(i)) / (double)msSinceLastLog * 1000.;
                msgIn.at(i) = msgInNew.at(i);

                bytesOutNew.at(i) = (*itr)->GetBytesTx();
                mbPerSecOut.at(i) = ((double)(bytesOutNew.at(i) - bytesOut.at(i)) / (1024. * 1024.)) / (double)msSinceLastLog * 1000.;
                bytesOut.at(i) = bytesOutNew.at(i);

                msgOutNew.at(i) = (*itr)->GetMessagesTx();
                msgPerSecOut.at(i) = (double)(msgOutNew.at(i) - msgOut.at(i)) / (double)msSinceLastLog * 1000.;
                msgOut.at(i) = msgOutNew.at(i);

                LOG(DEBUG) << filteredChannelNames.at(i) << ": "
                           << "in: " << msgPerSecIn.at(i) << " msg (" << mbPerSecIn.at(i) << " MB), "
                           << "out: " << msgPerSecOut.at(i) << " msg (" << mbPerSecOut.at(i) << " MB)";

                ++i;
            }

            t0 = t1;
            boost::this_thread::sleep(boost::posix_time::milliseconds(fLogIntervalInMs));
        }
        catch (boost::thread_interrupted&)
        {
            // LOG(DEBUG) << "FairMQDevice::LogSocketRates() interrupted";
            break;
        }
    }

    // LOG(DEBUG) << "FairMQDevice::LogSocketRates() stopping";
}

void FairMQDevice::InteractiveStateLoop()
{
    bool running = true;
    char c; // hold the user console input

    struct termios t;
    tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
    t.c_lflag &= ~ICANON; // disable canonical input
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

    PrintInteractiveStateLoopHelp();

    while (running && cin >> c)
    {
        switch (c)
        {
            case 'i':
                LOG(INFO) << "[i] init device";
                ChangeState("INIT_DEVICE");
                break;
            case 'j':
                LOG(INFO) << "[j] init task";
                ChangeState("INIT_TASK");
                break;
            case 'p':
                LOG(INFO) << "[p] pause";
                ChangeState("PAUSE");
                break;
            case 'r':
                LOG(INFO) << "[r] run";
                ChangeState("RUN");
                break;
            case 's':
                LOG(INFO) << "[s] stop";
                ChangeState("STOP");
                break;
            case 't':
                LOG(INFO) << "[t] reset task";
                ChangeState("RESET_TASK");
                break;
            case 'd':
                LOG(INFO) << "[d] reset device";
                ChangeState("RESET_DEVICE");
                break;
            case 'h':
                LOG(INFO) << "[h] help";
                PrintInteractiveStateLoopHelp();
                break;
            // case 'x':
            //     LOG(INFO) << "[x] ERROR";
            //     ChangeState("ERROR_FOUND");
            //     break;
            case 'q':
                LOG(INFO) << "[q] end";
                ChangeState("END");
                if (CheckCurrentState("EXITING"))
                {
                    running = false;
                }
                break;
            default:
                LOG(INFO) << "Invalid input: [" << c << "]";
                PrintInteractiveStateLoopHelp();
                break;
        }
    }

    tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
    t.c_lflag |= ICANON; // re-enable canonical input
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings
}

inline void FairMQDevice::PrintInteractiveStateLoopHelp()
{
    LOG(INFO) << "Use keys to control the state machine:";
    LOG(INFO) << "[h] help, [p] pause, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device";
}

void FairMQDevice::Unblock()
{
    FairMQMessage* cmd = fTransportFactory->CreateMessage();
    fCmdSocket->Send(cmd, 0);
    delete cmd;
}

void FairMQDevice::ResetTaskWrapper()
{
    ResetTask();

    ChangeState(internal_DEVICE_READY);

    // notify parent thread about end of processing.
    boost::lock_guard<boost::mutex> lock(fStateMutex);
    fStateFinished = true;
    fStateCondition.notify_one();
}

void FairMQDevice::ResetTask()
{
}

void FairMQDevice::ResetWrapper()
{
    Reset();

    ChangeState(internal_IDLE);

    // notify parent thread about end of processing.
    boost::lock_guard<boost::mutex> lock(fStateMutex);
    fStateFinished = true;
    fStateCondition.notify_one();
}

void FairMQDevice::Reset()
{
    // iterate over the channels map
    for (auto mi = fChannels.begin(); mi != fChannels.end(); ++mi)
    {
        // iterate over the channels vector
        for (auto vi = (mi->second).begin(); vi != (mi->second).end(); ++vi)
        {
            vi->fSocket->Close();
            delete vi->fSocket;
            vi->fSocket = nullptr;

            delete vi->fPoller;
            vi->fPoller = nullptr;

            vi->fCmdSocket->Close();
            delete vi->fCmdSocket;
            vi->fCmdSocket = nullptr;
        }
    }
}

void FairMQDevice::Terminate()
{
    // Termination signal has to be sent only once to any socket.
    if (fCmdSocket)
    {
        fCmdSocket->Terminate();
    }
}

void FairMQDevice::Shutdown()
{
    LOG(DEBUG) << "Closing sockets...";

    // iterate over the channels map
    for (auto mi = fChannels.begin(); mi != fChannels.end(); ++mi)
    {
        // iterate over the channels vector
        for (auto vi = (mi->second).begin(); vi != (mi->second).end(); ++vi)
        {
            if (vi->fSocket)
            {
                vi->fSocket->Close();
            }
            if (vi->fCmdSocket)
            {
                vi->fCmdSocket->Close();
            }
        }
    }

    if (fCmdSocket)
    {
        fCmdSocket->Close();
    }

    LOG(DEBUG) << "Closed all sockets!";
}

FairMQDevice::~FairMQDevice()
{
    // iterate over the channels map
    for (auto mi = fChannels.begin(); mi != fChannels.end(); ++mi)
    {
        // iterate over the channels vector
        for (auto vi = (mi->second).begin(); vi != (mi->second).end(); ++vi)
        {
            if (vi->fSocket)
            {
                delete vi->fSocket;
                vi->fSocket = nullptr;
            }
            if (vi->fPoller)
            {
                delete vi->fPoller;
                vi->fPoller = nullptr;
            }
        }
    }

    if (fCmdSocket)
    {
        delete fCmdSocket;
        fCmdSocket = nullptr;
    }

    delete fTransportFactory;
}

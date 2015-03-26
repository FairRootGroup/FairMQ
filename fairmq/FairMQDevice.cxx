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

#include <boost/thread.hpp>
#include <boost/random/mersenne_twister.hpp> // for choosing random port in range
#include <boost/random/uniform_int_distribution.hpp> // for choosing random port in range

#include "FairMQSocket.h"
#include "FairMQDevice.h"
#include "FairMQLogger.h"

using namespace std;

FairMQDevice::FairMQDevice()
    : fId()
    , fNumIoThreads(1)
    , fNumInputs(0)
    , fNumOutputs(0)
    , fPortRangeMin(22000)
    , fPortRangeMax(32000)
    , fInputAddress()
    , fInputMethod()
    , fInputSocketType()
    , fInputSndBufSize()
    , fInputRcvBufSize()
    , fInputRateLogging()
    , fOutputAddress()
    , fOutputMethod()
    , fOutputSocketType()
    , fOutputSndBufSize()
    , fOutputRcvBufSize()
    , fOutputRateLogging()
    , fPayloadInputs(new vector<FairMQSocket*>())
    , fPayloadOutputs(new vector<FairMQSocket*>())
    , fLogIntervalInMs(1000)
    , fTransportFactory(NULL)
{
}

void FairMQDevice::Init()
{
    LOG(INFO) << ">>>>>>> Init <<<<<<<";

    for (int i = 0; i < fNumInputs; ++i)
    {
        fInputAddress.push_back("ipc://default"); // default value, can be overwritten in configuration
        fInputMethod.push_back("connect"); // default value, can be overwritten in configuration
        fInputSocketType.push_back("sub"); // default value, can be overwritten in configuration
        fInputSndBufSize.push_back(10000); // default value, can be overwritten in configuration
        fInputRcvBufSize.push_back(10000); // default value, can be overwritten in configuration
        fInputRateLogging.push_back(1); // default value, can be overwritten in configuration
    }

    for (int i = 0; i < fNumOutputs; ++i)
    {
        fOutputAddress.push_back("ipc://default"); // default value, can be overwritten in configuration
        fOutputMethod.push_back("bind");    // default value, can be overwritten in configuration
        fOutputSocketType.push_back("pub"); // default value, can be overwritten in configuration
        fOutputSndBufSize.push_back(10000); // default value, can be overwritten in configuration
        fOutputRcvBufSize.push_back(10000); // default value, can be overwritten in configuration
        fOutputRateLogging.push_back(1); // default value, can be overwritten in configuration
    }
}

void FairMQDevice::InitInput()
{
    LOG(INFO) << ">>>>>>> InitInput <<<<<<<";

    for (int i = 0; i < fNumInputs; ++i)
    {
        FairMQSocket* socket = fTransportFactory->CreateSocket(fInputSocketType.at(i), i, fNumIoThreads);

        socket->SetOption("snd-hwm", &fInputSndBufSize.at(i), sizeof(fInputSndBufSize.at(i)));
        socket->SetOption("rcv-hwm", &fInputRcvBufSize.at(i), sizeof(fInputRcvBufSize.at(i)));

        fPayloadInputs->push_back(socket);
    }
}

void FairMQDevice::InitOutput()
{
    LOG(INFO) << ">>>>>>> InitOutput <<<<<<<";

    for (int i = 0; i < fNumOutputs; ++i)
    {
        FairMQSocket* socket = fTransportFactory->CreateSocket(fOutputSocketType.at(i), i, fNumIoThreads);

        socket->SetOption("snd-hwm", &fOutputSndBufSize.at(i), sizeof(fOutputSndBufSize.at(i)));
        socket->SetOption("rcv-hwm", &fOutputRcvBufSize.at(i), sizeof(fOutputRcvBufSize.at(i)));

        fPayloadOutputs->push_back(socket);
    }
}

void FairMQDevice::Bind()
{
    LOG(INFO) << ">>>>>>> binding <<<<<<<";

    int maxAttempts = 1000;
    int numAttempts = 0;

    boost::random::mt19937 gen(getpid());
    boost::random::uniform_int_distribution<> randomPort(fPortRangeMin, fPortRangeMax);

    for (int i = 0; i < fNumOutputs; ++i)
    {
        if (fOutputMethod.at(i) == "bind")
        {
            if (!fPayloadOutputs->at(i)->Bind(fOutputAddress.at(i)))
            {
                LOG(DEBUG) << "binding output #" << i << " on " << fOutputAddress.at(i) << " failed, trying to find port in range";
                LOG(INFO) << "port range: " << fPortRangeMin << " - " << fPortRangeMax;
                do {
                    ++numAttempts;

                    stringstream ss;
                    ss << (int)randomPort(gen);
                    string portString = ss.str();

                    size_t pos = fOutputAddress.at(i).rfind(":");
                    fOutputAddress.at(i) = fOutputAddress.at(i).substr(0, pos + 1) + portString;
                    if (numAttempts > maxAttempts)
                    {
                        LOG(ERROR) << "could not bind output " << i << " to any port in the given range";
                        break;
                    }
                } while (!fPayloadOutputs->at(i)->Bind(fOutputAddress.at(i)));
            }
        }
    }

    numAttempts = 0;

    for (int i = 0; i < fNumInputs; ++i)
    {
        if (fInputMethod.at(i) == "bind")
        {
            if (!fPayloadInputs->at(i)->Bind(fInputAddress.at(i)))
            {
                LOG(DEBUG) << "binding input #" << i << " on " << fInputAddress.at(i) << " failed, trying to find port in range";
                LOG(INFO) << "port range: " << fPortRangeMin << " - " << fPortRangeMax;
                do {
                    ++numAttempts;

                    stringstream ss;
                    ss << (int)randomPort(gen);
                    string portString = ss.str();

                    size_t pos = fInputAddress.at(i).rfind(":");
                    fInputAddress.at(i) = fInputAddress.at(i).substr(0, pos + 1) + portString;
                    if (numAttempts > maxAttempts)
                    {
                        LOG(ERROR) << "could not bind input " << i << " to any port in the given range";
                        break;
                    }
                } while (!fPayloadInputs->at(i)->Bind(fInputAddress.at(i)));
            }
        }
    }
}

void FairMQDevice::Connect()
{
    LOG(INFO) << ">>>>>>> connecting <<<<<<<";

    for (int i = 0; i < fNumOutputs; ++i)
    {
        if (fOutputMethod.at(i) == "connect")
        {
            fPayloadOutputs->at(i)->Connect(fOutputAddress.at(i));
        }
    }

    for (int i = 0; i < fNumInputs; ++i)
    {
        if (fInputMethod.at(i) == "connect")
        {
            fPayloadInputs->at(i)->Connect(fInputAddress.at(i));
        }
    }
}

void FairMQDevice::Run()
{
}

void FairMQDevice::Pause()
{
}

// Method for setting properties represented as a string.
void FairMQDevice::SetProperty(const int key, const string& value, const int slot /*= 0*/)
{
    switch (key)
    {
        case Id:
            fId = value;
            break;
        case InputAddress:
            fInputAddress.erase(fInputAddress.begin() + slot);
            fInputAddress.insert(fInputAddress.begin() + slot, value);
            break;
        case OutputAddress:
            fOutputAddress.erase(fOutputAddress.begin() + slot);
            fOutputAddress.insert(fOutputAddress.begin() + slot, value);
            break;
        case InputMethod:
            fInputMethod.erase(fInputMethod.begin() + slot);
            fInputMethod.insert(fInputMethod.begin() + slot, value);
            break;
        case OutputMethod:
            fOutputMethod.erase(fOutputMethod.begin() + slot);
            fOutputMethod.insert(fOutputMethod.begin() + slot, value);
            break;
        case InputSocketType:
            fInputSocketType.erase(fInputSocketType.begin() + slot);
            fInputSocketType.insert(fInputSocketType.begin() + slot, value);
            break;
        case OutputSocketType:
            fOutputSocketType.erase(fOutputSocketType.begin() + slot);
            fOutputSocketType.insert(fOutputSocketType.begin() + slot, value);
            break;
        default:
            FairMQConfigurable::SetProperty(key, value, slot);
            break;
    }
}

// Method for setting properties represented as an integer.
void FairMQDevice::SetProperty(const int key, const int value, const int slot /*= 0*/)
{
    switch (key)
    {
        case NumIoThreads:
            fNumIoThreads = value;
            break;
        case NumInputs:
            fNumInputs = value;
            break;
        case NumOutputs:
            fNumOutputs = value;
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
        case InputSndBufSize:
            fInputSndBufSize.erase(fInputSndBufSize.begin() + slot);
            fInputSndBufSize.insert(fInputSndBufSize.begin() + slot, value);
            break;
        case InputRcvBufSize:
            fInputRcvBufSize.erase(fInputRcvBufSize.begin() + slot);
            fInputRcvBufSize.insert(fInputRcvBufSize.begin() + slot, value);
            break;
        case InputRateLogging:
        case LogInputRate: // keep this for backwards compatibility for a while
            fInputRateLogging.erase(fInputRateLogging.begin() + slot);
            fInputRateLogging.insert(fInputRateLogging.begin() + slot, value);
            break;
        case OutputSndBufSize:
            fOutputSndBufSize.erase(fOutputSndBufSize.begin() + slot);
            fOutputSndBufSize.insert(fOutputSndBufSize.begin() + slot, value);
            break;
        case OutputRcvBufSize:
            fOutputRcvBufSize.erase(fOutputRcvBufSize.begin() + slot);
            fOutputRcvBufSize.insert(fOutputRcvBufSize.begin() + slot, value);
            break;
        case OutputRateLogging:
        case LogOutputRate: // keep this for backwards compatibility for a while
            fOutputRateLogging.erase(fOutputRateLogging.begin() + slot);
            fOutputRateLogging.insert(fOutputRateLogging.begin() + slot, value);
            break;
        default:
            FairMQConfigurable::SetProperty(key, value, slot);
            break;
    }
}

// Method for getting properties represented as an string.
string FairMQDevice::GetProperty(const int key, const string& default_ /*= ""*/, const int slot /*= 0*/)
{
    switch (key)
    {
        case Id:
            return fId;
        case InputAddress:
            return fInputAddress.at(slot);
        case OutputAddress:
            return fOutputAddress.at(slot);
        case InputMethod:
            return fInputMethod.at(slot);
        case OutputMethod:
            return fOutputMethod.at(slot);
        case InputSocketType:
            return fInputSocketType.at(slot);
        case OutputSocketType:
            return fOutputSocketType.at(slot);
        default:
            return FairMQConfigurable::GetProperty(key, default_, slot);
    }
}

// Method for getting properties represented as an integer.
int FairMQDevice::GetProperty(const int key, const int default_ /*= 0*/, const int slot /*= 0*/)
{
    switch (key)
    {
        case NumIoThreads:
            return fNumIoThreads;
        case NumInputs:
            return fNumInputs;
        case NumOutputs:
            return fNumOutputs;
        case PortRangeMin:
            return fPortRangeMin;
        case PortRangeMax:
            return fPortRangeMax;
        case LogIntervalInMs:
            return fLogIntervalInMs;
        case InputSndBufSize:
            return fInputSndBufSize.at(slot);
        case InputRcvBufSize:
            return fInputRcvBufSize.at(slot);
        case InputRateLogging:
        case LogInputRate: // keep this for backwards compatibility for a while
            return fInputRateLogging.at(slot);
        case OutputSndBufSize:
            return fOutputSndBufSize.at(slot);
        case OutputRcvBufSize:
            return fOutputRcvBufSize.at(slot);
        case OutputRateLogging:
        case LogOutputRate: // keep this for backwards compatibility for a while
            return fOutputRateLogging.at(slot);
        default:
            return FairMQConfigurable::GetProperty(key, default_, slot);
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

    int numFilteredInputs = 0;
    int numFilteredOutputs = 0;
    vector<FairMQSocket*> filteredInputs;
    vector<FairMQSocket*> filteredOutputs;

    int i = 0;
    for (vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++)
    {
        if (fInputRateLogging.at(i) > 0)
        {
            filteredInputs.push_back((*itr));
            ++numFilteredInputs;
        }
        ++i;
    }

    i = 0;
    for (vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++)
    {
        if (fOutputRateLogging.at(i) > 0)
        {
            filteredOutputs.push_back((*itr));
            ++numFilteredOutputs;
        }
        ++i;
    }

    vector<unsigned long> bytesIn(numFilteredInputs);
    vector<unsigned long> msgIn(numFilteredInputs);
    vector<unsigned long> bytesOut(numFilteredOutputs);
    vector<unsigned long> msgOut(numFilteredOutputs);

    vector<unsigned long> bytesInNew(numFilteredInputs);
    vector<unsigned long> msgInNew(numFilteredInputs);
    vector<unsigned long> bytesOutNew(numFilteredOutputs);
    vector<unsigned long> msgOutNew(numFilteredOutputs);

    vector<double> mbPerSecIn(numFilteredInputs);
    vector<double> msgPerSecIn(numFilteredInputs);
    vector<double> mbPerSecOut(numFilteredOutputs);
    vector<double> msgPerSecOut(numFilteredOutputs);

    i = 0;
    for (vector<FairMQSocket*>::iterator itr = filteredInputs.begin(); itr != filteredInputs.end(); itr++)
    {
        bytesIn.at(i) = (*itr)->GetBytesRx();
        msgIn.at(i) = (*itr)->GetMessagesRx();
        ++i;
    }

    i = 0;
    for (vector<FairMQSocket*>::iterator itr = filteredOutputs.begin(); itr != filteredOutputs.end(); itr++)
    {
        bytesOut.at(i) = (*itr)->GetBytesTx();
        msgOut.at(i) = (*itr)->GetMessagesTx();
        ++i;
    }

    t0 = get_timestamp();

    while (fState == RUNNING)
    {
        try
        {
            t1 = get_timestamp();

            msSinceLastLog = (t1 - t0) / 1000.0L;

            i = 0;

            for (vector<FairMQSocket*>::iterator itr = filteredInputs.begin(); itr != filteredInputs.end(); itr++)
            {
                bytesInNew.at(i) = (*itr)->GetBytesRx();
                mbPerSecIn.at(i) = ((double)(bytesInNew.at(i) - bytesIn.at(i)) / (1024. * 1024.)) / (double)msSinceLastLog * 1000.;
                bytesIn.at(i) = bytesInNew.at(i);
                msgInNew.at(i) = (*itr)->GetMessagesRx();
                msgPerSecIn.at(i) = (double)(msgInNew.at(i) - msgIn.at(i)) / (double)msSinceLastLog * 1000.;
                msgIn.at(i) = msgInNew.at(i);

                LOG(DEBUG) << "#" << fId << "." << (*itr)->GetId() << ": " << msgPerSecIn.at(i) << " msg/s, " << mbPerSecIn.at(i) << " MB/s";

                ++i;
            }

            i = 0;

            for (vector<FairMQSocket*>::iterator itr = filteredOutputs.begin(); itr != filteredOutputs.end(); itr++)
            {
                bytesOutNew.at(i) = (*itr)->GetBytesTx();
                mbPerSecOut.at(i) = ((double)(bytesOutNew.at(i) - bytesOut.at(i)) / (1024. * 1024.)) / (double)msSinceLastLog * 1000.;
                bytesOut.at(i) = bytesOutNew.at(i);
                msgOutNew.at(i) = (*itr)->GetMessagesTx();
                msgPerSecOut.at(i) = (double)(msgOutNew.at(i) - msgOut.at(i)) / (double)msSinceLastLog * 1000.;
                msgOut.at(i) = msgOutNew.at(i);

                LOG(DEBUG) << "#" << fId << "." << (*itr)->GetId() << ": " << msgPerSecOut.at(i) << " msg/s, " << mbPerSecOut.at(i) << " MB/s";

                ++i;
            }

            t0 = t1;
            boost::this_thread::sleep(boost::posix_time::milliseconds(fLogIntervalInMs));
        }
        catch (boost::thread_interrupted&)
        {
            LOG(INFO) << "FairMQDevice::LogSocketRates() interrupted";
            break;
        }
    }

    LOG(INFO) << ">>>>>>> stopping FairMQDevice::LogSocketRates() <<<<<<<";
}

void FairMQDevice::Shutdown()
{
    LOG(INFO) << ">>>>>>> closing inputs <<<<<<<";
    for (vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++)
    {
        (*itr)->Close();
    }

    LOG(INFO) << ">>>>>>> closing outputs <<<<<<<";
    for (vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++)
    {
        (*itr)->Close();
    }
}

void FairMQDevice::Terminate()
{
    // Termination signal has to be sent only once to any socket.
    // Find available socket and send termination signal to it.
    if (fPayloadInputs->size() > 0)
    {
        fPayloadInputs->at(0)->Terminate();
    }
    else if (fPayloadOutputs->size() > 0)
    {
        fPayloadOutputs->at(0)->Terminate();
    }
    else
    {
        LOG(ERROR) << "No sockets available to terminate.";
    }
}

FairMQDevice::~FairMQDevice()
{
    for (vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++)
    {
        delete (*itr);
    }

    for (vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++)
    {
        delete (*itr);
    }

    delete fPayloadInputs;
    delete fPayloadOutputs;
}

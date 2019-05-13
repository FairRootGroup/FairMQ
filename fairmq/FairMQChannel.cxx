/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "FairMQChannel.h"
#include <fairmq/Tools.h>

#include <boost/algorithm/string.hpp> // join/split

#include <set>
#include <random>

using namespace std;

mutex FairMQChannel::fChannelMutex;

FairMQChannel::FairMQChannel()
    : FairMQChannel("", "unspecified", "unspecified", "unspecified", nullptr)
{}

FairMQChannel::FairMQChannel(const string& type, const string& method, const string& address)
    : FairMQChannel("", type, method, address, nullptr)
{}

FairMQChannel::FairMQChannel(const string& name, const string& type, shared_ptr<FairMQTransportFactory> factory)
    : FairMQChannel(name, type, "unspecified", "unspecified", factory)
{}

FairMQChannel::FairMQChannel(const string& name, const string& type, const string& method, const string& address, shared_ptr<FairMQTransportFactory> factory)
    : fTransportFactory(factory)
    , fTransportType(factory ? factory->GetType() : fair::mq::Transport::DEFAULT)
    , fSocket(factory ? factory->CreateSocket(type, name) : nullptr)
    , fType(type)
    , fMethod(method)
    , fAddress(address)
    , fSndBufSize(1000)
    , fRcvBufSize(1000)
    , fSndKernelSize(0)
    , fRcvKernelSize(0)
    , fLinger(500)
    , fRateLogging(1)
    , fPortRangeMin(22000)
    , fPortRangeMax(23000)
    , fAutoBind(true)
    , fName(name)
    , fIsValid(false)
    , fMultipart(false)
    , fModified(true)
    , fReset(false)
{}

FairMQChannel::FairMQChannel(const FairMQChannel& chan)
    : fTransportFactory(nullptr)
    , fTransportType(chan.fTransportType)
    , fSocket(nullptr)
    , fType(chan.fType)
    , fMethod(chan.fMethod)
    , fAddress(chan.fAddress)
    , fSndBufSize(chan.fSndBufSize)
    , fRcvBufSize(chan.fRcvBufSize)
    , fSndKernelSize(chan.fSndKernelSize)
    , fRcvKernelSize(chan.fRcvKernelSize)
    , fLinger(chan.fLinger)
    , fRateLogging(chan.fRateLogging)
    , fPortRangeMin(chan.fPortRangeMin)
    , fPortRangeMax(chan.fPortRangeMax)
    , fAutoBind(chan.fAutoBind)
    , fName(chan.fName)
    , fIsValid(false)
    , fMultipart(chan.fMultipart)
    , fModified(chan.fModified)
    , fReset(false)
{}

FairMQChannel& FairMQChannel::operator=(const FairMQChannel& chan)
{
    fTransportFactory = nullptr;
    fTransportType = chan.fTransportType;
    fSocket = nullptr;
    fType = chan.fType;
    fMethod = chan.fMethod;
    fAddress = chan.fAddress;
    fSndBufSize = chan.fSndBufSize;
    fRcvBufSize = chan.fRcvBufSize;
    fSndKernelSize = chan.fSndKernelSize;
    fRcvKernelSize = chan.fRcvKernelSize;
    fLinger = chan.fLinger;
    fRateLogging = chan.fRateLogging;
    fPortRangeMin = chan.fPortRangeMin;
    fPortRangeMax = chan.fPortRangeMax;
    fAutoBind = chan.fAutoBind;
    fName = chan.fName;
    fIsValid = false;
    fMultipart = chan.fMultipart;
    fModified = chan.fModified;
    fReset = false;

    return *this;
}

FairMQSocket & FairMQChannel::GetSocket() const
{
    assert(fSocket);
    return *fSocket;
}

string FairMQChannel::GetName() const
{
    lock_guard<mutex> lock(fChannelMutex);
    return fName;
}

string FairMQChannel::GetPrefix() const
{
    lock_guard<mutex> lock(fChannelMutex);
    string prefix = fName;
    prefix = prefix.erase(fName.rfind('['));
    return prefix;
}

string FairMQChannel::GetIndex() const
{
    lock_guard<mutex> lock(fChannelMutex);
    string indexStr = fName;
    indexStr.erase(indexStr.rfind(']'));
    indexStr.erase(0, indexStr.rfind('[') + 1);
    return indexStr;
}

string FairMQChannel::GetType() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fType;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetType: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

string FairMQChannel::GetMethod() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fMethod;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetMethod: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

string FairMQChannel::GetAddress() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fAddress;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetAddress: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

string FairMQChannel::GetTransportName() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fair::mq::TransportNames.at(fTransportType);
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetTransportName: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetSndBufSize() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fSndBufSize;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetSndBufSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetRcvBufSize() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fRcvBufSize;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetRcvBufSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetSndKernelSize() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fSndKernelSize;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetSndKernelSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetRcvKernelSize() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fRcvKernelSize;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetRcvKernelSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetLinger() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fLinger;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetLinger: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetRateLogging() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fRateLogging;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetRateLogging: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetPortRangeMin() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fPortRangeMin;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetPortRangeMin: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

int FairMQChannel::GetPortRangeMax() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fPortRangeMax;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetPortRangeMax: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

bool FairMQChannel::GetAutoBind() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fAutoBind;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::GetAutoBind: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateType(const string& type)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fType = type;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateType: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateMethod(const string& method)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fMethod = method;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateMethod: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateAddress(const string& address)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fAddress = address;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateAddress: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateTransport(const string& transport)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fTransportType = fair::mq::TransportTypes.at(transport);
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateTransport: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateSndBufSize(const int sndBufSize)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fSndBufSize = sndBufSize;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateSndBufSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateRcvBufSize(const int rcvBufSize)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fRcvBufSize = rcvBufSize;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateRcvBufSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateSndKernelSize(const int sndKernelSize)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fSndKernelSize = sndKernelSize;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateSndKernelSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateRcvKernelSize(const int rcvKernelSize)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fRcvKernelSize = rcvKernelSize;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateRcvKernelSize: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateLinger(const int duration)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fLinger = duration;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateLinger: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateRateLogging(const int rateLogging)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fRateLogging = rateLogging;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateRateLogging: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdatePortRangeMin(const int minPort)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fPortRangeMin = minPort;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdatePortRangeMin: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdatePortRangeMax(const int maxPort)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fPortRangeMax = maxPort;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdatePortRangeMax: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateAutoBind(const bool autobind)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fAutoBind = autobind;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateAutoBind: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

auto FairMQChannel::SetModified(const bool modified) -> void
try {
    lock_guard<mutex> lock(fChannelMutex);
    fModified = modified;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::SetModified: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

void FairMQChannel::UpdateName(const string& name)
try {
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    fName = name;
    fModified = true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::UpdateName: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

bool FairMQChannel::IsValid() const
try {
    lock_guard<mutex> lock(fChannelMutex);
    return fIsValid;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::IsValid: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString("failed to acquire lock: ", e.what()));
}

bool FairMQChannel::Validate()
try {
    lock_guard<mutex> lock(fChannelMutex);
    stringstream ss;
    ss << "Validating channel '" << fName << "'... ";

    if (fIsValid) {
        ss << "ALREADY VALID";
        LOG(debug) << ss.str();
        return true;
    }

    // validate socket type
    const set<string> socketTypes{ "sub", "pub", "pull", "push", "req", "rep", "xsub", "xpub", "dealer", "router", "pair" };
    if (socketTypes.find(fType) == socketTypes.end()) {
        ss << "INVALID";
        LOG(debug) << ss.str();
        LOG(error) << "Invalid channel type: '" << fType << "'";
        throw ChannelConfigurationError(fair::mq::tools::ToString("Invalid channel type: '", fType, "'"));
    }

    // validate socket address
    if (fAddress == "unspecified" || fAddress == "") {
        ss << "INVALID";
        LOG(debug) << ss.str();
        LOG(debug) << "invalid channel address: '" << fAddress << "'";
        return false;
    } else {
        vector<string> endpoints;
        boost::algorithm::split(endpoints, fAddress, boost::algorithm::is_any_of(";"));
        for (const auto endpoint : endpoints) {
            string address;
            if (endpoint[0] == '@' || endpoint[0] == '+' || endpoint[0] == '>') {
                address = endpoint.substr(1);
            } else {
                // we don't have a method modifier, check if the default method is set
                const set<string> socketMethods{ "bind", "connect" };
                if (socketMethods.find(fMethod) == socketMethods.end()) {
                    ss << "INVALID";
                    LOG(debug) << ss.str();
                    LOG(error) << "Invalid endpoint connection method: '" << fMethod << "' for " << endpoint;
                    throw ChannelConfigurationError(fair::mq::tools::ToString("Invalid endpoint connection method: '", fMethod, "' for ", endpoint));
                }
                address = endpoint;
            }
            // check if address is a tcp or ipc address
            if (address.compare(0, 6, "tcp://") == 0) {
                // check if TCP address contains port delimiter
                string addressString = address.substr(6);
                if (addressString.find(':') == string::npos) {
                    ss << "INVALID";
                    LOG(debug) << ss.str();
                    LOG(error) << "invalid channel address: '" << address << "' (missing port?)";
                    return false;
                }
            } else if (address.compare(0, 6, "ipc://") == 0) {
                // check if IPC address is not empty
                string addressString = address.substr(6);
                if (addressString == "") {
                    ss << "INVALID";
                    LOG(debug) << ss.str();
                    LOG(error) << "invalid channel address: '" << address << "' (empty IPC address?)";
                    return false;
                }
            } else if (address.compare(0, 9, "inproc://") == 0) {
                // check if IPC address is not empty
                string addressString = address.substr(9);
                if (addressString == "") {
                    ss << "INVALID";
                    LOG(debug) << ss.str();
                    LOG(error) << "invalid channel address: '" << address << "' (empty inproc address?)";
                    return false;
                }
            } else if (address.compare(0, 8, "verbs://") == 0) {
                // check if IPC address is not empty
                string addressString = address.substr(8);
                if (addressString == "") {
                    ss << "INVALID";
                    LOG(debug) << ss.str();
                    LOG(error) << "invalid channel address: '" << address << "' (empty verbs address?)";
                    return false;
                }
            } else {
                // if neither TCP or IPC is specified, return invalid
                ss << "INVALID";
                LOG(debug) << ss.str();
                LOG(error) << "invalid channel address: '" << address << "' (missing protocol specifier?)";
                return false;
            }
        }
    }

    // validate socket buffer size for sending
    if (fSndBufSize < 0) {
        ss << "INVALID";
        LOG(debug) << ss.str();
        LOG(error) << "invalid channel send buffer size (cannot be negative): '" << fSndBufSize << "'";
        throw ChannelConfigurationError(fair::mq::tools::ToString("invalid channel send buffer size (cannot be negative): '", fSndBufSize, "'"));
    }

    // validate socket buffer size for receiving
    if (fRcvBufSize < 0) {
        ss << "INVALID";
        LOG(debug) << ss.str();
        LOG(error) << "invalid channel receive buffer size (cannot be negative): '" << fRcvBufSize << "'";
        throw ChannelConfigurationError(fair::mq::tools::ToString("invalid channel receive buffer size (cannot be negative): '", fRcvBufSize, "'"));
    }

    // validate socket kernel transmit size for sending
    if (fSndKernelSize < 0) {
        ss << "INVALID";
        LOG(debug) << ss.str();
        LOG(error) << "invalid channel send kernel transmit size (cannot be negative): '" << fSndKernelSize << "'";
        throw ChannelConfigurationError(fair::mq::tools::ToString("invalid channel send kernel transmit size (cannot be negative): '", fSndKernelSize, "'"));
    }

    // validate socket kernel transmit size for receiving
    if (fRcvKernelSize < 0) {
        ss << "INVALID";
        LOG(debug) << ss.str();
        LOG(error) << "invalid channel receive kernel transmit size (cannot be negative): '" << fRcvKernelSize << "'";
        throw ChannelConfigurationError(fair::mq::tools::ToString("invalid channel receive kernel transmit size (cannot be negative): '", fRcvKernelSize, "'"));
    }

    // validate socket rate logging interval
    if (fRateLogging < 0) {
        ss << "INVALID";
        LOG(debug) << ss.str();
        LOG(error) << "invalid socket rate logging interval (cannot be negative): '" << fRateLogging << "'";
        throw ChannelConfigurationError(fair::mq::tools::ToString("invalid socket rate logging interval (cannot be negative): '", fRateLogging, "'"));
    }

    fIsValid = true;
    ss << "VALID";
    LOG(debug) << ss.str();
    return true;
} catch (exception& e) {
    LOG(error) << "Exception caught in FairMQChannel::ValidateChannel: " << e.what();
    throw ChannelConfigurationError(fair::mq::tools::ToString(e.what()));
}

void FairMQChannel::Init()
{
    lock_guard<mutex> lock(fChannelMutex);

    fSocket = fTransportFactory->CreateSocket(fType, fName);

    // set linger duration (how long socket should wait for outstanding transfers before shutdown)
    fSocket->SetLinger(fLinger);

    // set high water marks
    fSocket->SetSndBufSize(fSndBufSize);
    fSocket->SetRcvBufSize(fRcvBufSize);

    // set kernel transmit size (set it only if value is not the default value)
    if (fSndKernelSize != 0) {
        fSocket->SetSndKernelSize(fSndKernelSize);
    }
    if (fRcvKernelSize != 0) {
        fSocket->SetRcvKernelSize(fRcvKernelSize);
    }
}

bool FairMQChannel::ConnectEndpoint(const string& endpoint)
{
    lock_guard<mutex> lock(fChannelMutex);

    return fSocket->Connect(endpoint);
}

bool FairMQChannel::BindEndpoint(string& endpoint)
{
    lock_guard<mutex> lock(fChannelMutex);

    // try to bind to the configured port. If it fails, try random one (if AutoBind is on).
    if (fSocket->Bind(endpoint)) {
        return true;
    } else {
        if (fAutoBind) {
            // number of attempts when choosing a random port
            int numAttempts = 0;
            int maxAttempts = 1000;

            // initialize random generator
            default_random_engine generator(chrono::system_clock::now().time_since_epoch().count());
            uniform_int_distribution<int> randomPort(fPortRangeMin, fPortRangeMax);

            do {
                LOG(debug) << "Could not bind to configured (TCP) port (" << endpoint << "), trying random port in range " << fPortRangeMin << "-" << fPortRangeMax;
                ++numAttempts;

                if (numAttempts > maxAttempts) {
                    LOG(error) << "could not bind to any (TCP) port in the given range after " << maxAttempts << " attempts";
                    return false;
                }

                size_t pos = endpoint.rfind(':');
                endpoint = endpoint.substr(0, pos + 1) + fair::mq::tools::ToString(static_cast<int>(randomPort(generator)));
            } while (!fSocket->Bind(endpoint));

            return true;
        } else {
            return false;
        }
    }

}

void FairMQChannel::ResetChannel()
{
    lock_guard<mutex> lock(fChannelMutex);
    fIsValid = false;
    // TODO: implement channel resetting
}

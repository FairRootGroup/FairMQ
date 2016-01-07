/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSink.cxx
 *
 * @since 2013-01-09
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/timer/timer.hpp>

#include "FairMQSink.h"
#include "FairMQLogger.h"

using namespace std;

FairMQSink::FairMQSink()
    : fNumMsgs(0)
{
}

void FairMQSink::Run()
{
    int numReceivedMsgs = 0;
    // store the channel reference to avoid traversing the map on every loop iteration
    const FairMQChannel& dataInChannel = fChannels.at("data-in").at(0);

    LOG(INFO) << "Starting the benchmark and expecting to receive " << fNumMsgs << " messages.";
    boost::timer::auto_cpu_timer timer;

    while (CheckCurrentState(RUNNING))
    {
        std::unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());

        if (dataInChannel.Receive(msg) >= 0)
        {
            if (fNumMsgs > 0)
            {
                numReceivedMsgs++;
                if (numReceivedMsgs >= fNumMsgs)
                {
                    break;
                }
            }
        }
    }

    LOG(INFO) << "Received " << numReceivedMsgs << " messages, leaving RUNNING state.";
    LOG(INFO) << "Receiving time: ";
}

FairMQSink::~FairMQSink()
{
}

void FairMQSink::SetProperty(const int key, const string& value)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

string FairMQSink::GetProperty(const int key, const string& default_ /*= ""*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

void FairMQSink::SetProperty(const int key, const int value)
{
    switch (key)
    {
        case NumMsgs:
            fNumMsgs = value;
            break;
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

int FairMQSink::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        case NumMsgs:
            return fNumMsgs;
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

string FairMQSink::GetPropertyDescription(const int key)
{
    switch (key)
    {
        case NumMsgs:
            return "NumMsgs: Number of messages to send.";
        default:
            return FairMQDevice::GetPropertyDescription(key);
    }
}

void FairMQSink::ListProperties()
{
    LOG(INFO) << "Properties of FairMQSink:";
    for (int p = FairMQConfigurable::Last; p < FairMQSink::Last; ++p)
    {
        LOG(INFO) << " " << GetPropertyDescription(p);
    }
    LOG(INFO) << "---------------------------";
}

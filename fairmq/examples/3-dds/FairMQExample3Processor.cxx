/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample3Processor.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExample3Processor.h"
#include "FairMQLogger.h"

FairMQExample3Processor::FairMQExample3Processor()
    : fTaskIndex(0)
{
}

void FairMQExample3Processor::CustomCleanup(void *data, void *object)
{
    delete (std::string*)object;
}

void FairMQExample3Processor::Run()
{
    while (CheckCurrentState(RUNNING))
    {
        FairMQMessage* input = fTransportFactory->CreateMessage();
        fChannels.at("data-in").at(0).Receive(input);

        LOG(INFO) << "Received data, processing...";

        std::string* text = new std::string(static_cast<char*>(input->GetData()), input->GetSize());
        *text += " (modified by " + fId + std::to_string(fTaskIndex) + ")";

        delete input;

        FairMQMessage* msg = fTransportFactory->CreateMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text);
        fChannels.at("data-out").at(0).Send(msg);
    }
}

void FairMQExample3Processor::SetProperty(const int key, const std::string& value)
{
    switch (key)
    {
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

std::string FairMQExample3Processor::GetProperty(const int key, const std::string& default_ /*= ""*/)
{
    switch (key)
    {
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

void FairMQExample3Processor::SetProperty(const int key, const int value)
{
    switch (key)
    {
        case TaskIndex:
            fTaskIndex = value;
            break;
        default:
            FairMQDevice::SetProperty(key, value);
            break;
    }
}

int FairMQExample3Processor::GetProperty(const int key, const int default_ /*= 0*/)
{
    switch (key)
    {
        case TaskIndex:
            return fTaskIndex;
            break;
        default:
            return FairMQDevice::GetProperty(key, default_);
    }
}

FairMQExample3Processor::~FairMQExample3Processor()
{
}

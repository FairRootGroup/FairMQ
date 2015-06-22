/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample2Processor.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExample2Processor.h"
#include "FairMQLogger.h"

FairMQExample2Processor::FairMQExample2Processor()
    : fText()
{
}

void FairMQExample2Processor::CustomCleanup(void *data, void *object)
{
    delete (std::string*)object;
}

void FairMQExample2Processor::Run()
{
    while (GetCurrentState() == RUNNING)
    {
        FairMQMessage* input = fTransportFactory->CreateMessage();
        fChannels["data-in"].at(0).Receive(input);

        LOG(INFO) << "Received data, processing...";

        std::string* text = new std::string(static_cast<char*>(input->GetData()), input->GetSize());
        *text += " (modified by " + fId + ")";

        delete input;

        FairMQMessage* msg = fTransportFactory->CreateMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text);
        fChannels["data-out"].at(0).Send(msg);
    }
}

FairMQExample2Processor::~FairMQExample2Processor()
{
}

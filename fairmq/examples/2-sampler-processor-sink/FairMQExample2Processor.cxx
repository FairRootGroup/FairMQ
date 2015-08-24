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

using namespace std;

FairMQExample2Processor::FairMQExample2Processor()
    : fText()
{
}

void FairMQExample2Processor::CustomCleanup(void *data, void *object)
{
    delete (string*)object;
}

void FairMQExample2Processor::Run()
{
    // Check if we are still in the RUNNING state
    while (CheckCurrentState(RUNNING))
    {
        // Create empty message to hold the input
        unique_ptr<FairMQMessage> input(fTransportFactory->CreateMessage());

        // Receive the message (blocks until received or interrupted (e.g. by state change)). 
        // Returns size of the received message or -1 if interrupted.
        if (fChannels.at("data-in").at(0).Receive(input) > 0)
        {
            LOG(INFO) << "Received data, processing...";

            // Modify the received string
            string* text = new string(static_cast<char*>(input->GetData()), input->GetSize());
            *text += " (modified by " + fId + ")";

            // Create output message
            unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text));

            // Send out the output message
            fChannels.at("data-out").at(0).Send(msg);
        }
    }
}

FairMQExample2Processor::~FairMQExample2Processor()
{
}

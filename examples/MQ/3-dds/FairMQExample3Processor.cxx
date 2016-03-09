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

using namespace std;

FairMQExample3Processor::FairMQExample3Processor()
{
}

void FairMQExample3Processor::CustomCleanup(void *data, void *object)
{
    delete static_cast<string*>(object);
}

void FairMQExample3Processor::Run()
{
    while (CheckCurrentState(RUNNING))
    {
        // Create empty message to hold the input
        unique_ptr<FairMQMessage> input(NewMessage());

        // Receive the message (blocks until received or interrupted (e.g. by state change)). 
        // Returns size of the received message or -1 if interrupted.
        if (Receive(input, "data1") >= 0)
        {
            LOG(INFO) << "Received data, processing...";

            // Modify the received string
            string* text = new string(static_cast<char*>(input->GetData()), input->GetSize());
            *text += " (modified by " + fId + ")";

            // Create output message
            unique_ptr<FairMQMessage> msg(NewMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text));

            // Send out the output message
            Send(msg, "data2");
        }
    }
}

FairMQExample3Processor::~FairMQExample3Processor()
{
}

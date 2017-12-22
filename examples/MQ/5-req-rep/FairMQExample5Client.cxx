/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample5Client.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <memory> // unique_ptr
#include <thread> // this_thread::sleep_for
#include <chrono>

#include "FairMQExample5Client.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h"

using namespace std;

FairMQExample5Client::FairMQExample5Client()
    : fText()
    , fMaxIterations(0)
    , fNumIterations(0)
{
}

void FairMQExample5Client::InitTask()
{
    fText = fConfig->GetValue<string>("text");
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
}

bool FairMQExample5Client::ConditionalRun()
{

    string* text = new string(fText);

    // create message object with a pointer to the data buffer,
    // its size,
    // custom deletion function (called when transfer is done),
    // and pointer to the object managing the data buffer
    FairMQMessagePtr request(NewMessage(const_cast<char*>(text->c_str()), // data
                                                          text->length(), // size
                                                          [](void* /*data*/, void* object) { delete static_cast<string*>(object); }, // deletion callback
                                                          text)); // object that manages the data
    FairMQMessagePtr reply(NewMessage());

    LOG(info) << "Sending \"" << fText << "\" to server.";

    if (Send(request, "data") > 0)
    {
        if (Receive(reply, "data") >= 0)
        {
            LOG(info) << "Received reply from server: \"" << string(static_cast<char*>(reply->GetData()), reply->GetSize()) << "\"";

            if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
            {
                LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
                return false;
            }

            this_thread::sleep_for(chrono::seconds(1));

            return true;
        }
    }

    return false;
}

FairMQExample5Client::~FairMQExample5Client()
{
}

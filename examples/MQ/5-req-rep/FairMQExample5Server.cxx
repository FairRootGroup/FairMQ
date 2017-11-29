/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample5Server.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "FairMQExample5Server.h"
#include "FairMQLogger.h"
#include "FairMQProgOptions.h" // device->fConfig

using namespace std;

FairMQExample5Server::FairMQExample5Server()
    : fMaxIterations(0)
    , fNumIterations(0)
{
    OnData("data", &FairMQExample5Server::HandleData);
}

void FairMQExample5Server::InitTask()
{
    // Get the fMaxIterations value from the command line options (via fConfig)
    fMaxIterations = fConfig->GetValue<uint64_t>("max-iterations");
}

bool FairMQExample5Server::HandleData(FairMQMessagePtr& request, int /*index*/)
{
    LOG(INFO) << "Received request from client: \"" << string(static_cast<char*>(request->GetData()), request->GetSize()) << "\"";

    string* text = new string("Thank you for the \"" + string(static_cast<char*>(request->GetData()), request->GetSize()) + "\"!");

    LOG(INFO) << "Sending reply to client.";

    FairMQMessagePtr reply(NewMessage(const_cast<char*>(text->c_str()), // data
                                                        text->length(), // size
                                                        [](void* /*data*/, void* object) { delete static_cast<string*>(object); }, // deletion callback
                                                        text)); // object that manages the data

    if (Send(reply, "data") > 0)
    {
        if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
        {
            LOG(INFO) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
            return false;
        }

        return true;
    }

    return false;
}

FairMQExample5Server::~FairMQExample5Server()
{
}

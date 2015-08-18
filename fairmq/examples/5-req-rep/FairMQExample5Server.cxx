/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample5Server.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExample5Server.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample5Server::FairMQExample5Server()
{
}

void FairMQExample5Server::CustomCleanup(void *data, void *hint)
{
    delete (string*)hint;
}

void FairMQExample5Server::Run()
{
    while (CheckCurrentState(RUNNING))
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        FairMQMessage* request = fTransportFactory->CreateMessage();

        if (fChannels.at("data").at(0).Receive(request) > 0)
        {
            LOG(INFO) << "Received request from client: \"" << string(static_cast<char*>(request->GetData()), request->GetSize()) << "\"";

            string* text = new string("Thank you for the \"" + string(static_cast<char*>(request->GetData()), request->GetSize()) + "\"!");

            delete request;

            LOG(INFO) << "Sending reply to client.";

            FairMQMessage* reply = fTransportFactory->CreateMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text);

            fChannels.at("data").at(0).Send(reply);
        }
    }
}

FairMQExample5Server::~FairMQExample5Server()
{
}

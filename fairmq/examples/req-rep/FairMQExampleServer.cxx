/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExampleServer.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExampleServer.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExampleServer::FairMQExampleServer()
{
}

void FairMQExampleServer::CustomCleanup(void *data, void *hint)
{
    delete (string*)hint;
}

void FairMQExampleServer::Run()
{
    while (GetCurrentState() == RUNNING)
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        FairMQMessage* request = fTransportFactory->CreateMessage();

        fChannels["data"].at(0).Receive(request);

        LOG(INFO) << "Received request from client: \"" << string(static_cast<char*>(request->GetData()), request->GetSize()) << "\"";

        string* text = new string("Thank you for the \"" + string(static_cast<char*>(request->GetData()), request->GetSize()) + "\"!");

        delete request;

        LOG(INFO) << "Sending reply to client.";

        FairMQMessage* reply = fTransportFactory->CreateMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text);

        fChannels["data"].at(0).Send(reply);
    }
}

FairMQExampleServer::~FairMQExampleServer()
{
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Server.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "Server.h"

using namespace std;

namespace example_req_rep
{

Server::Server()
    : fMaxIterations(0)
    , fNumIterations(0)
{
    OnData("data", &Server::HandleData);
}

void Server::InitTask()
{
    // Get the fMaxIterations value from the command line options (via fConfig)
    fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
}

bool Server::HandleData(FairMQMessagePtr& req, int /*index*/)
{
    LOG(info) << "Received request from client: \"" << string(static_cast<char*>(req->GetData()), req->GetSize()) << "\"";

    string* text = new string("Thank you for the \"" + string(static_cast<char*>(req->GetData()), req->GetSize()) + "\"!");

    LOG(info) << "Sending reply to client.";

    FairMQMessagePtr rep(NewMessage(const_cast<char*>(text->c_str()), // data
                                                        text->length(), // size
                                                        [](void* /*data*/, void* object) { delete static_cast<string*>(object); }, // deletion callback
                                                        text)); // object that manages the data

    if (Send(rep, "data") > 0)
    {
        if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations)
        {
            LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
            return false;
        }

        return true;
    }

    return false;
}

Server::~Server()
{
}

} // namespace example_req_rep

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <string>

namespace bpo = boost::program_options;

struct Server : fair::mq::Device
{
    Server()
    {
        OnData("data", &Server::HandleData);
    }

    void InitTask() override
    {
        // Get the fMaxIterations value from the command line options (via fConfig)
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
    }

    bool HandleData(fair::mq::MessagePtr& req, int)
    {
        LOG(info) << "Received request from client: \"" << std::string(static_cast<char*>(req->GetData()), req->GetSize()) << "\"";

        std::string* text = new std::string("Thank you for the \"" + std::string(static_cast<char*>(req->GetData()), req->GetSize()) + "\"!");

        LOG(info) << "Sending reply to client.";

        fair::mq::MessagePtr rep(NewMessage(const_cast<char*>(text->c_str()), // data
                                                            text->length(), // size
                                                            [](void* /*data*/, void* object) { delete static_cast<std::string*>(object); }, // deletion callback
                                                            text)); // object that manages the data

        if (Send(rep, "data") > 0) {
            if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
                LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
                return false;
            }

            return true;
        }

        return false;
    }

  private:
    uint64_t fMaxIterations = 0;
    uint64_t fNumIterations = 0;
};

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Server>();
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <chrono>
#include <string>
#include <thread> // this_thread::sleep_for

namespace bpo = boost::program_options;

struct Client : fair::mq::Device
{
    void InitTask() override
    {
        fText = fConfig->GetProperty<std::string>("text");
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
    }

    bool ConditionalRun() override
    {

        std::string* text = new std::string(fText);

        // create message object with a pointer to the data buffer,
        // its size,
        // custom deletion function (called when transfer is done),
        // and pointer to the object managing the data buffer
        fair::mq::MessagePtr req(NewMessage(const_cast<char*>(text->c_str()), // data
                                                            text->length(), // size
                                                            [](void* /*data*/, void* object) { delete static_cast<std::string*>(object); }, // deletion callback
                                                            text)); // object that manages the data
        fair::mq::MessagePtr rep(NewMessage());

        LOG(info) << "Sending \"" << fText << "\" to server.";

        if (Send(req, "data") > 0) {
            if (Receive(rep, "data") >= 0) {
                LOG(info) << "Received reply from server: \"" << std::string(static_cast<char*>(rep->GetData()), rep->GetSize()) << "\"";

                if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
                    LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
                    return false;
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));

                return true;
            }
        }

        return false;
    }

  private:
    std::string fText;
    uint64_t fMaxIterations = 0;
    uint64_t fNumIterations = 0;
};

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("text", bpo::value<std::string>()->default_value("Hello"), "Text to send out")
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Client>();
}

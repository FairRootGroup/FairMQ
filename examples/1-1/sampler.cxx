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

struct Sampler : fair::mq::Device
{
    void InitTask() override
    {
        // Get the fText and fMaxIterations values from the command line options (via fConfig)
        fText = fConfig->GetProperty<std::string>("text");
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
    }

    bool ConditionalRun() override
    {
        // create a copy of the data with new(), that will be deleted after the transfer is complete
        std::string* text = new std::string(fText);

        // create message object with a pointer to the data buffer, its size,
        // custom deletion function (called when transfer is done),
        // and pointer to the object managing the data buffer
        fair::mq::MessagePtr msg(NewMessage(
            const_cast<char*>(text->c_str()),
            text->length(),
            [](void* /*data*/, void* object) { delete static_cast<std::string*>(object); },
            text));

        LOG(info) << "Sending \"" << fText << "\"";

        // in case of error or transfer interruption, return false to go to the Ready state
        // successfull transfer will return number of bytes transfered (can be 0 if sending an empty message).
        if (Send(msg, "data") < 0) {
            return false;
        } else if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
            LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
            return false;
        }

        return true;
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
    return std::make_unique<Sampler>();
}

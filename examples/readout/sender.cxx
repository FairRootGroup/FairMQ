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

struct Sender : fair::mq::Device
{
    void Init() override
    {
        fInputChannelName = fConfig->GetProperty<std::string>("input-name");
        OnData(fInputChannelName, &Sender::HandleData);
    }

    bool HandleData(fair::mq::MessagePtr& msg, int /*index*/)
    {
        if (Send(msg, "sr") < 0) {
            return false;
        }

        return true;
    }

  private:
    std::string fInputChannelName;
};

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("input-name", bpo::value<std::string>()->default_value("bs"), "Input channel name");
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Sender>();
}

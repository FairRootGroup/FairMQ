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

class Builder : public fair::mq::Device
{
  public:
    Builder() = default;

    void Init() override
    {
        fOutputChannelName = fConfig->GetProperty<std::string>("output-name");
        OnData("rb", &Builder::HandleData);
    }

    bool HandleData(fair::mq::MessagePtr& msg, int /*index*/)
    {
        if (Send(msg, fOutputChannelName) < 0) {
            return false;
        }

        return true;
    }

  private:
    std::string fOutputChannelName;
};

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("output-name", bpo::value<std::string>()->default_value("bs"), "Output channel name");
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Builder>();
}

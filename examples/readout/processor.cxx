/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

namespace bpo = boost::program_options;

struct Processor : fair::mq::Device
{
    Processor()
    {
        OnData("bp", &Processor::HandleData);
    }

    bool HandleData(FairMQMessagePtr& msg, int /*index*/)
    {
        FairMQMessagePtr msg2(NewMessageFor("ps", 0, msg->GetSize()));
        if (Send(msg2, "ps") < 0) {
            return false;
        }

        return true;
    }
};

void addCustomOptions(bpo::options_description& /* options */)
{}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Processor>();
}

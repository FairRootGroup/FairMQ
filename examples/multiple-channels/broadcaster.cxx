/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <thread> // this_thread::sleep_for
#include <chrono>

namespace bpo = boost::program_options;

struct Broadcaster : fair::mq::Device
{
    bool ConditionalRun() override
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // NewSimpleMessage creates a copy of the data and takes care of its destruction (after the transfer takes place).
        // Should only be used for small data because of the cost of an additional copy
        FairMQMessagePtr msg(NewSimpleMessage("OK"));

        LOG(info) << "Sending OK";

        Send(msg, "broadcast");

        return true;
    }
};

void addCustomOptions(bpo::options_description& /*options*/)
{
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Broadcaster>();
}

/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/devices/Proxy.h>
#include <fairmq/runDevice.h>

namespace bpo = boost::program_options;

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("in-channel", bpo::value<std::string>()->default_value("data-in"), "Name of the input channel")
        ("out-channel", bpo::value<std::string>()->default_value("data-out"), "Name of the output channel")
        ("multipart", bpo::value<bool>()->default_value(true), "Handle multipart payloads");
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<fair::mq::Proxy>();
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <runFairMQDevice.h>
#include <devices/FairMQMerger.h>

namespace bpo = boost::program_options;

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("in-channel", bpo::value<std::string>()->default_value("data-in"), "Name of the input channel")
        ("out-channel", bpo::value<std::string>()->default_value("data-out"), "Name of the output channel")
        ("multipart", bpo::value<bool>()->default_value(true), "Handle multipart payloads");
}

FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /*config*/)
{
    return new FairMQMerger();
}

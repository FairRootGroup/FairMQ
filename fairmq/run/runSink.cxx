/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <runFairMQDevice.h>
#include <fairmq/devices/FairMQSink.h>

namespace bpo = boost::program_options;

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("in-channel", bpo::value<std::string>()->default_value("data"), "Name of the input channel")
        ("out-filename", bpo::value<std::string>()->default_value(""), "Write incoming message buffers to the specified file")
        ("max-file-size", bpo::value<uint64_t>()->default_value(2000000000), "Maximum file size for the file output (0 - unlimited)")
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Number of run iterations (0 - infinite)")
        ("multipart", bpo::value<bool>()->default_value(false), "Handle multipart payloads");
}

FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /*config*/)
{
    return new FairMQSink();
}

/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <runFairMQDevice.h>
#include <devices/FairMQBenchmarkSampler.h>

namespace bpo = boost::program_options;

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("out-channel", bpo::value<std::string>()->default_value("data"), "Name of the output channel")
        ("same-msg", bpo::value<bool>()->default_value(false), "Re-send the same message, or recreate for each iteration")
        ("multipart", bpo::value<bool>()->default_value(false), "Handle multipart payloads")
        ("num-parts", bpo::value<size_t>()->default_value(1), "Number of parts to send. 1 will send single messages, not parts")
        ("msg-size", bpo::value<size_t>()->default_value(1000000), "Message size in bytes")
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Number of run iterations (0 - infinite)")
        ("msg-rate", bpo::value<float>()->default_value(0), "Msg rate limit in maximum number of messages per second");
}

FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /* config */)
{
    return new FairMQBenchmarkSampler();
}

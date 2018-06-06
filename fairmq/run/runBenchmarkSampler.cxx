/********************************************************************************
 *  Copyright (C) 2014-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH *
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
        ("msg-size", bpo::value<int>()->default_value(1000000), "Message size in bytes")
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Number of run iterations (0 - infinite)")
        ("msg-rate", bpo::value<int>()->default_value(0), "Msg rate limit in maximum number of messages per second");
}

FairMQDevicePtr getDevice(const FairMQProgOptions& /*config*/)
{
    return new FairMQBenchmarkSampler();
}

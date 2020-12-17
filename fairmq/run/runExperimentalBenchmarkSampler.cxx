/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <runFairMQDevice.h>
#include <devices/ExperimentalBenchmarkSampler.h>

namespace bpo = boost::program_options;

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("out-channel", bpo::value<std::string>()->default_value("data"), "Name of the output channel")
        ("memset", bpo::value<bool>()->default_value(false), "Memset allocated buffers to 0")
        ("num-buffers", bpo::value<size_t>()->default_value(1), "Number of buffers to send.")
        ("buf-size", bpo::value<size_t>()->default_value(1000000), "Buffer size in bytes")
        ("buf-alignment", bpo::value<size_t>()->default_value(0), "Buffer alignment")
        ("max-iterations", bpo::value<uint64_t>()->default_value(0), "Number of run iterations (0 - infinite)")
        ("msg-rate", bpo::value<float>()->default_value(0), "Msg rate limit in maximum number of messages per second");
}

FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /* config */)
{
    return new fair::mq::ExperimentalBenchmarkSampler();
}

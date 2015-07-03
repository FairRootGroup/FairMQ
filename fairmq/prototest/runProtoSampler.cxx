/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runBenchmarkSampler.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQProtoSampler.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using namespace std;

typedef struct DeviceOptions
{
    DeviceOptions() :
        id(), eventSize(0), eventRate(0), ioThreads(0),
        outputSocketType(), outputBufSize(0), outputMethod(), outputAddress() {}

    string id;
    int eventSize;
    int eventRate;
    int ioThreads;
    string outputSocketType;
    int outputBufSize;
    string outputMethod;
    string outputAddress;
} DeviceOptions_t;

inline bool parse_cmd_line(int _argc, char* _argv[], DeviceOptions* _options)
{
    if (_options == NULL)
        throw runtime_error("Internal error: options' container is empty.");

    namespace bpo = boost::program_options;
    bpo::options_description desc("Options");
    desc.add_options()
        ("id", bpo::value<string>()->required(), "Device ID")
        ("event-size", bpo::value<int>()->default_value(1000), "Event size in bytes")
        ("event-rate", bpo::value<int>()->default_value(0), "Event rate limit in maximum number of events per second")
        ("io-threads", bpo::value<int>()->default_value(1), "Number of I/O threads")
        ("output-socket-type", bpo::value<string>()->required(), "Output socket type: pub/push")
        ("output-buff-size", bpo::value<int>()->required(), "Output buffer size in number of messages (ZeroMQ)/bytes(nanomsg)")
        ("output-method", bpo::value<string>()->required(), "Output method: bind/connect")
        ("output-address", bpo::value<string>()->required(), "Output address, e.g.: \"tcp://*:5555\"")
        ("help", "Print help messages");

    bpo::variables_map vm;
    bpo::store(bpo::parse_command_line(_argc, _argv, desc), vm);

    if ( vm.count("help") )
    {
        LOG(INFO) << "FairMQ Proto Sampler" << endl << desc;
        return false;
    }

    bpo::notify(vm);

    if ( vm.count("id") )
        _options->id = vm["id"].as<string>();

    if ( vm.count("event-size") )
        _options->eventSize = vm["event-size"].as<int>();

    if ( vm.count("event-rate") )
        _options->eventRate = vm["event-rate"].as<int>();

    if ( vm.count("io-threads") )
        _options->ioThreads = vm["io-threads"].as<int>();

    if ( vm.count("output-socket-type") )
        _options->outputSocketType = vm["output-socket-type"].as<string>();

    if ( vm.count("output-buff-size") )
        _options->outputBufSize = vm["output-buff-size"].as<int>();

    if ( vm.count("output-method") )
        _options->outputMethod = vm["output-method"].as<string>();

    if ( vm.count("output-address") )
        _options->outputAddress = vm["output-address"].as<string>();

    return true;
}

int main(int argc, char** argv)
{
    FairMQProtoSampler sampler;
    sampler.CatchSignals();

    DeviceOptions_t options;
    try
    {
        if (!parse_cmd_line(argc, argv, &options))
            return 0;
    }
    catch (exception& e)
    {
        LOG(ERROR) << e.what();
        return 1;
    }

    LOG(INFO) << "PID: " << getpid();
    LOG(INFO) << "CONFIG: " << "id: " << options.id << ", event size: " << options.eventSize << ", event rate: " << options.eventRate << ", I/O threads: " << options.ioThreads;
    LOG(INFO) << "OUTPUT: " << options.outputSocketType << " " << options.outputBufSize << " " << options.outputMethod << " " << options.outputAddress;

#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

    sampler.SetTransport(transportFactory);

    FairMQChannel outputChannel(options.outputSocketType, options.outputMethod, options.outputAddress);
    outputChannel.UpdateSndBufSize(options.outputBufSize);
    outputChannel.UpdateRcvBufSize(options.outputBufSize);
    outputChannel.UpdateRateLogging(1);

    sampler.fChannels["data-out"].push_back(outputChannel);

    sampler.SetProperty(FairMQProtoSampler::Id, options.id);
    sampler.SetProperty(FairMQProtoSampler::EventSize, options.eventSize);
    sampler.SetProperty(FairMQProtoSampler::EventRate, options.eventRate);
    sampler.SetProperty(FairMQProtoSampler::NumIoThreads, options.ioThreads);

    sampler.ChangeState("INIT_DEVICE");
    sampler.WaitForEndOfState("INIT_DEVICE");

    sampler.ChangeState("INIT_TASK");
    sampler.WaitForEndOfState("INIT_TASK");

    sampler.ChangeState("RUN");
    sampler.InteractiveStateLoop();

    return 0;
}

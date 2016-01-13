/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runBuffer.cxx
 *
 * @since 2012-10-26
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQBuffer.h"

using namespace std;

typedef struct DeviceOptions
{
    DeviceOptions() :
        id(), ioThreads(0), transport(),
        inputSocketType(), inputBufSize(0), inputMethod(), inputAddress(),
        outputSocketType(), outputBufSize(0), outputMethod(), outputAddress() {}

    string id;
    int ioThreads;
    string transport;
    string inputSocketType;
    int inputBufSize;
    string inputMethod;
    string inputAddress;
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
        ("id", bpo::value<string>(), "Device ID")
        ("io-threads", bpo::value<int>()->default_value(1), "Number of I/O threads")
        ("transport", bpo::value<string>()->default_value("zeromq"), "Transport (zeromq/nanomsg)")
        ("input-socket-type", bpo::value<string>()->required(), "Input socket type: sub/pull")
        ("input-buff-size", bpo::value<int>()->default_value(1000), "Input buffer size in number of messages (ZeroMQ)/bytes(nanomsg)")
        ("input-method", bpo::value<string>()->required(), "Input method: bind/connect")
        ("input-address", bpo::value<string>()->required(), "Input address, e.g.: \"tcp://localhost:5555\"")
        ("output-socket-type", bpo::value<string>()->required(), "Output socket type: pub/push")
        ("output-buff-size", bpo::value<int>()->default_value(1000), "Output buffer size in number of messages (ZeroMQ)/bytes(nanomsg)")
        ("output-method", bpo::value<string>()->required(), "Output method: bind/connect")
        ("output-address", bpo::value<string>()->required(), "Output address, e.g.: \"tcp://localhost:5555\"")
        ("help", "Print help messages");

    bpo::variables_map vm;
    bpo::store(bpo::parse_command_line(_argc, _argv, desc), vm);

    if (vm.count("help"))
    {
        LOG(INFO) << "FairMQ Buffer" << endl << desc;
        return false;
    }

    bpo::notify(vm);

    if (vm.count("id")) { _options->id = vm["id"].as<string>(); }
    if (vm.count("io-threads")) { _options->ioThreads = vm["io-threads"].as<int>(); }
    if (vm.count("transport"))  { _options->transport = vm["transport"].as<string>(); }
    if (vm.count("input-socket-type")) { _options->inputSocketType = vm["input-socket-type"].as<string>(); }
    if (vm.count("input-buff-size")) { _options->inputBufSize = vm["input-buff-size"].as<int>(); }
    if (vm.count("input-method")) { _options->inputMethod = vm["input-method"].as<string>(); }
    if (vm.count("input-address")) { _options->inputAddress = vm["input-address"].as<string>(); }
    if (vm.count("output-socket-type")) { _options->outputSocketType = vm["output-socket-type"].as<string>(); }
    if (vm.count("output-buff-size")) { _options->outputBufSize = vm["output-buff-size"].as<int>(); }
    if (vm.count("output-method")) { _options->outputMethod = vm["output-method"].as<string>(); }
    if (vm.count("output-address")) { _options->outputAddress = vm["output-address"].as<string>(); }

    return true;
}

int main(int argc, char** argv)
{
    FairMQBuffer buffer;
    buffer.CatchSignals();

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

    buffer.SetTransport(options.transport);

    FairMQChannel inputChannel(options.inputSocketType, options.inputMethod, options.inputAddress);
    inputChannel.UpdateSndBufSize(options.inputBufSize);
    inputChannel.UpdateRcvBufSize(options.inputBufSize);
    inputChannel.UpdateRateLogging(1);

    buffer.fChannels["data-in"].push_back(inputChannel);

    FairMQChannel outputChannel(options.outputSocketType, options.outputMethod, options.outputAddress);
    outputChannel.UpdateSndBufSize(options.outputBufSize);
    outputChannel.UpdateRcvBufSize(options.outputBufSize);
    outputChannel.UpdateRateLogging(1);

    buffer.fChannels["data-out"].push_back(outputChannel);

    buffer.SetProperty(FairMQBuffer::Id, options.id);
    buffer.SetProperty(FairMQBuffer::NumIoThreads, options.ioThreads);

    buffer.ChangeState("INIT_DEVICE");
    buffer.WaitForEndOfState("INIT_DEVICE");

    buffer.ChangeState("INIT_TASK");
    buffer.WaitForEndOfState("INIT_TASK");

    buffer.ChangeState("RUN");
    buffer.InteractiveStateLoop();

    return 0;
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runExampleClient.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQExampleClient.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using namespace std;

FairMQExampleClient client;

static void s_signal_handler(int signal)
{
    LOG(INFO) << "Caught signal " << signal;

    client.ChangeState(FairMQExampleClient::END);

    LOG(INFO) << "Shutdown complete.";
    exit(1);
}

static void s_catch_signals(void)
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

typedef struct DeviceOptions
{
    DeviceOptions() :
        text() {}

    string text;
} DeviceOptions_t;

inline bool parse_cmd_line(int _argc, char* _argv[], DeviceOptions* _options)
{
    if (_options == NULL)
        throw runtime_error("Internal error: options' container is empty.");

    namespace bpo = boost::program_options;
    bpo::options_description desc("Options");
    desc.add_options()
        ("text,t", bpo::value<string>()->default_value("something"), "Text to send to server")
        ("help", "Print help messages");

    bpo::variables_map vm;
    bpo::store(bpo::parse_command_line(_argc, _argv, desc), vm);

    if ( vm.count("help") )
    {
        LOG(INFO) << "EPN" << endl << desc;
        return false;
    }

    bpo::notify(vm);

    if ( vm.count("text") )
        _options->text = vm["text"].as<string>();

    return true;
}

int main(int argc, char** argv)
{
    s_catch_signals();

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

#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

    client.SetTransport(transportFactory);

    client.SetProperty(FairMQExampleClient::Id, "client");
    client.SetProperty(FairMQExampleClient::NumIoThreads, 1);

    FairMQChannel requestChannel("req", "connect", "tcp://localhost:5005");
    requestChannel.fSndBufSize = 10000;
    requestChannel.fRcvBufSize = 10000;
    requestChannel.fRateLogging = 1;

    client.fChannels["data"].push_back(requestChannel);

    client.ChangeState(FairMQExampleClient::INIT_DEVICE);
    client.WaitForEndOfState(FairMQExampleClient::INIT_DEVICE);

    client.ChangeState(FairMQExampleClient::INIT_TASK);
    client.WaitForEndOfState(FairMQExampleClient::INIT_TASK);

    client.ChangeState(FairMQExampleClient::RUN);
    client.WaitForEndOfState(FairMQExampleClient::RUN);

    client.ChangeState(FairMQExampleClient::STOP);

    client.ChangeState(FairMQExampleClient::RESET_TASK);
    client.WaitForEndOfState(FairMQExampleClient::RESET_TASK);

    client.ChangeState(FairMQExampleClient::RESET_DEVICE);
    client.WaitForEndOfState(FairMQExampleClient::RESET_DEVICE);

    client.ChangeState(FairMQExampleClient::END);

    return 0;
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runSink.cxx
 *
 * @since 2013-01-21
 * @author: D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"
#include "FairMQSink.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using namespace std;
using namespace FairMQParser;
using namespace boost::program_options;

FairMQSink sink;

static void s_signal_handler(int signal)
{
    LOG(INFO) << "Caught signal " << signal;

    sink.ChangeState(FairMQSink::END);

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

int main(int argc, char** argv)
{
    s_catch_signals();

    FairMQProgOptions config;

    try
    {
        int ioThreads;

        options_description sink_options("Sink options");
        sink_options.add_options()
            ("io-threads", value<int>(&ioThreads)->default_value(1),    "Number of I/O threads");

        config.AddToCmdLineOptions(sink_options);

        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        string filename = config.GetValue<string>("config-json-filename");
        string id = config.GetValue<string>("device-id");

        config.UserParser<JSON>(filename, id);

        sink.fChannels = config.GetFairMQMap();

        LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

        sink.SetTransport(transportFactory);

        sink.SetProperty(FairMQSink::Id, id);
        sink.SetProperty(FairMQSink::NumIoThreads, ioThreads);

        sink.ChangeState(FairMQSink::INIT_DEVICE);
        sink.WaitForEndOfState(FairMQSink::INIT_DEVICE);

        sink.ChangeState(FairMQSink::INIT_TASK);
        sink.WaitForEndOfState(FairMQSink::INIT_TASK);

        sink.ChangeState(FairMQSink::RUN);
        sink.WaitForEndOfState(FairMQSink::RUN);

        sink.ChangeState(FairMQSink::STOP);

        sink.ChangeState(FairMQSink::RESET_TASK);
        sink.WaitForEndOfState(FairMQSink::RESET_TASK);

        sink.ChangeState(FairMQSink::RESET_DEVICE);
        sink.WaitForEndOfState(FairMQSink::RESET_DEVICE);

        sink.ChangeState(FairMQSink::END);

    }
    catch (exception& e)
    {
        LOG(ERROR) << e.what();
        LOG(INFO) << "Started with: ";
        config.PrintHelp();
        return 1;
    }

    return 0;
}

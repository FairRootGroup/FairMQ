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
 * @author: D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"
#include "FairMQBenchmarkSampler.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using namespace std;
using namespace FairMQParser;
using namespace boost::program_options;

FairMQBenchmarkSampler sampler;

static void s_signal_handler(int signal)
{
    LOG(INFO) << "Caught signal " << signal;

    sampler.ChangeState(FairMQBenchmarkSampler::END);

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
        int eventSize;
        int eventRate;
        int ioThreads;

        options_description sampler_options("Sampler options");
        sampler_options.add_options()
            ("event-size", value<int>(&eventSize)->default_value(1000), "Event size in bytes")
            ("event-rate", value<int>(&eventRate)->default_value(0),    "Event rate limit in maximum number of events per second")
            ("io-threads", value<int>(&ioThreads)->default_value(1),    "Number of I/O threads");

        config.AddToCmdLineOptions(sampler_options);

        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        string filename = config.GetValue<string>("config-json-filename");
        string id = config.GetValue<string>("device-id");

        config.UserParser<JSON>(filename, id);

        sampler.fChannels = config.GetFairMQMap();

        LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

        sampler.SetTransport(transportFactory);

        sampler.SetProperty(FairMQBenchmarkSampler::Id, id);
        sampler.SetProperty(FairMQBenchmarkSampler::EventSize, eventSize);
        sampler.SetProperty(FairMQBenchmarkSampler::EventRate, eventRate);
        sampler.SetProperty(FairMQBenchmarkSampler::NumIoThreads, ioThreads);

        sampler.ChangeState(FairMQBenchmarkSampler::INIT_DEVICE);
        sampler.WaitForEndOfState(FairMQBenchmarkSampler::INIT_DEVICE);

        sampler.ChangeState(FairMQBenchmarkSampler::INIT_TASK);
        sampler.WaitForEndOfState(FairMQBenchmarkSampler::INIT_TASK);

        sampler.ChangeState(FairMQBenchmarkSampler::RUN);
        sampler.WaitForEndOfState(FairMQBenchmarkSampler::RUN);

        sampler.ChangeState(FairMQBenchmarkSampler::STOP);

        sampler.ChangeState(FairMQBenchmarkSampler::RESET_TASK);
        sampler.WaitForEndOfState(FairMQBenchmarkSampler::RESET_TASK);

        sampler.ChangeState(FairMQBenchmarkSampler::RESET_DEVICE);
        sampler.WaitForEndOfState(FairMQBenchmarkSampler::RESET_DEVICE);

        sampler.ChangeState(FairMQBenchmarkSampler::END);

    }
    catch (exception& e)
    {
        LOG(ERROR) << e.what();
        LOG(INFO) << "Command line options are the following : ";
        config.PrintHelp();
        return 1;
    }

    return 0;
}

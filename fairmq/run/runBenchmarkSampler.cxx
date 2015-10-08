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
#include "FairMQTools.h"
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

int main(int argc, char** argv)
{
    FairMQBenchmarkSampler sampler;
    sampler.CatchSignals();

    FairMQProgOptions config;

    try
    {
        int eventSize;
        int eventRate;

        options_description sampler_options("Sampler options");
        sampler_options.add_options()
            ("event-size", value<int>(&eventSize)->default_value(1000), "Event size in bytes")
            ("event-rate", value<int>(&eventRate)->default_value(0),    "Event rate limit in maximum number of events per second");

        config.AddToCmdLineOptions(sampler_options);

        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        string filename = config.GetValue<string>("config-json-file");
        string id = config.GetValue<string>("id");
        int ioThreads = config.GetValue<int>("io-threads");

        config.UserParser<JSON>(filename, id);

        sampler.fChannels = config.GetFairMQMap();

        LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
        sampler.SetTransport(new FairMQTransportFactoryNN());
#else
        sampler.SetTransport(new FairMQTransportFactoryZMQ());
#endif

        sampler.SetProperty(FairMQBenchmarkSampler::Id, id);
        sampler.SetProperty(FairMQBenchmarkSampler::EventSize, eventSize);
        sampler.SetProperty(FairMQBenchmarkSampler::EventRate, eventRate);
        sampler.SetProperty(FairMQBenchmarkSampler::NumIoThreads, ioThreads);

        sampler.ChangeState("INIT_DEVICE");
        sampler.WaitForEndOfState("INIT_DEVICE");

        sampler.ChangeState("INIT_TASK");
        sampler.WaitForEndOfState("INIT_TASK");

        sampler.ChangeState("RUN");
        sampler.InteractiveStateLoop();
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

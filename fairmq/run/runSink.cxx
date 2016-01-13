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

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"
#include "FairMQSink.h"

using namespace std;
using namespace FairMQParser;
using namespace boost::program_options;

int main(int argc, char** argv)
{
    FairMQSink sink;
    sink.CatchSignals();

    FairMQProgOptions config;

    try
    {
        int numMsgs;

        options_description sink_options("Sink options");
        sink_options.add_options()
            ("num-msgs", value<int>(&numMsgs)->default_value(0), "Number of messages to receive");

        config.AddToCmdLineOptions(sink_options);

        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        string filename = config.GetValue<string>("config-json-file");
        string id = config.GetValue<string>("id");
        int ioThreads = config.GetValue<int>("io-threads");

        config.UserParser<JSON>(filename, id);

        sink.fChannels = config.GetFairMQMap();

        LOG(INFO) << "PID: " << getpid();

        sink.SetTransport(config.GetValue<std::string>("transport"));

        sink.SetProperty(FairMQSink::Id, id);
        sink.SetProperty(FairMQSink::NumMsgs, numMsgs);
        sink.SetProperty(FairMQSink::NumIoThreads, config.GetValue<int>("io-threads"));

        sink.ChangeState("INIT_DEVICE");
        sink.WaitForEndOfState("INIT_DEVICE");

        sink.ChangeState("INIT_TASK");
        sink.WaitForEndOfState("INIT_TASK");

        sink.ChangeState("RUN");
        sink.InteractiveStateLoop();
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

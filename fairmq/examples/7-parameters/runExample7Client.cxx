/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runExample7Client.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>

#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"
#include "FairMQExample7Client.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using namespace std;
using namespace boost::program_options;

int main(int argc, char** argv)
{
    FairMQExample7Client client;
    client.CatchSignals();

    FairMQProgOptions config;

    try
    {
        string parameterName;

        options_description clientOptions("Parameter Client options");
        clientOptions.add_options()
            ("parameter-name", value<string>(&parameterName)->default_value("FairMQExample7ParOne"), "Parameter Name");

        config.AddToCmdLineOptions(clientOptions);

        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        string file = config.GetValue<string>("config-json-file");
        string id = config.GetValue<string>("id");

        config.UserParser<FairMQParser::JSON>(file, id);

        client.fChannels = config.GetFairMQMap();

        LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

        client.SetTransport(transportFactory);

        client.SetProperty(FairMQExample7Client::Id, "client");
        client.SetProperty(FairMQExample7Client::ParameterName, parameterName);
        client.SetProperty(FairMQExample7Client::NumIoThreads, 1);

        FairMQChannel requestChannel("req", "connect", "tcp://localhost:5005");
        requestChannel.UpdateSndBufSize(10000);
        requestChannel.UpdateRcvBufSize(10000);
        requestChannel.UpdateRateLogging(0);

        client.fChannels["data"].push_back(requestChannel);

        client.ChangeState("INIT_DEVICE");
        client.WaitForEndOfState("INIT_DEVICE");

        client.ChangeState("INIT_TASK");
        client.WaitForEndOfState("INIT_TASK");

        client.ChangeState("RUN");
        client.InteractiveStateLoop();
    }
    catch (exception& e)
    {
        LOG(ERROR) << e.what();
        LOG(INFO) << "Command line options are the following: ";
        config.PrintHelp();
        return 1;
    }

    return 0;
}

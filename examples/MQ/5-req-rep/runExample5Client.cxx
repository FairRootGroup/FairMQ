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

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"
#include "FairMQExample5Client.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using namespace std;
using namespace boost::program_options;

int main(int argc, char** argv)
{
    FairMQExample5Client client;
    client.CatchSignals();

    FairMQProgOptions config;

    try
    {
        string text;

        options_description clientOptions("Client options");
        clientOptions.add_options()
            ("text", value<string>(&text)->default_value("Hello"), "Text to send out");

        config.AddToCmdLineOptions(clientOptions);

        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        string filename = config.GetValue<string>("config-json-file");
        string id = config.GetValue<string>("id");

        config.UserParser<FairMQParser::JSON>(filename, id);

        client.fChannels = config.GetFairMQMap();

        LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

        client.SetTransport(transportFactory);

        client.SetProperty(FairMQExample5Client::Id, id);
        client.SetProperty(FairMQExample5Client::Text, text);

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

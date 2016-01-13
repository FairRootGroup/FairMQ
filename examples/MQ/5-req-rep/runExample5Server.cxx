/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runExampleServer.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"
#include "FairMQExample5Server.h"

using namespace std;
using namespace boost::program_options;

int main(int argc, char** argv)
{
    FairMQExample5Server server;
    server.CatchSignals();

    FairMQProgOptions config;

    try
    {
        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        string filename = config.GetValue<string>("config-json-file");
        string id = config.GetValue<string>("id");

        config.UserParser<FairMQParser::JSON>(filename, id);

        server.fChannels = config.GetFairMQMap();

        LOG(INFO) << "PID: " << getpid();

        server.SetTransport(config.GetValue<std::string>("transport"));

        server.SetProperty(FairMQExample5Server::Id, id);

        server.ChangeState("INIT_DEVICE");
        server.WaitForEndOfState("INIT_DEVICE");

        server.ChangeState("INIT_TASK");
        server.WaitForEndOfState("INIT_TASK");

        server.ChangeState("RUN");
        server.InteractiveStateLoop();
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

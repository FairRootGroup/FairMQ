/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runExample4Sampler.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>

#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"
#include "FairMQExample4Sampler.h"

int main(int argc, char** argv)
{
    FairMQExample4Sampler sampler;
    sampler.CatchSignals();

    FairMQProgOptions config;

    try
    {
        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        std::string filename = config.GetValue<std::string>("config-json-file");
        std::string id = config.GetValue<std::string>("id");

        config.UserParser<FairMQParser::JSON>(filename, id);

        sampler.fChannels = config.GetFairMQMap();

        LOG(INFO) << "PID: " << getpid();

        sampler.SetTransport(config.GetValue<std::string>("transport"));

        sampler.SetProperty(FairMQExample4Sampler::Id, id);

        sampler.ChangeState("INIT_DEVICE");
        sampler.WaitForEndOfState("INIT_DEVICE");

        sampler.ChangeState("INIT_TASK");
        sampler.WaitForEndOfState("INIT_TASK");

        sampler.ChangeState("RUN");
        sampler.InteractiveStateLoop();
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << e.what();
        LOG(INFO) << "Command line options are the following: ";
        config.PrintHelp();
        return 1;
    }

    return 0;
}

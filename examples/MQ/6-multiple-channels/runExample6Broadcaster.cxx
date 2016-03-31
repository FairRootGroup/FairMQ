/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runExample6Broadcaster.cxx
 *
 * @since 2013-04-23
 * @author A. Rybalchenko
 */

#include <iostream>

#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"
#include "FairMQExample6Broadcaster.h"

int main(int argc, char** argv)
{
    FairMQExample6Broadcaster broadcaster;
    broadcaster.CatchSignals();

    FairMQProgOptions config;

    try
    {
        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        broadcaster.SetConfig(config);

        broadcaster.ChangeState("INIT_DEVICE");
        broadcaster.WaitForEndOfState("INIT_DEVICE");

        broadcaster.ChangeState("INIT_TASK");
        broadcaster.WaitForEndOfState("INIT_TASK");

        broadcaster.ChangeState("RUN");
        broadcaster.InteractiveStateLoop();
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

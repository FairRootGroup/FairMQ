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

#include "FairMQLogger.h"
#include "FairMQProgOptions.h"
#include "FairMQExample6Broadcaster.h"

int main(int argc, char** argv)
{
    try
    {
        FairMQProgOptions config;
        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        FairMQExample6Broadcaster broadcaster;
        broadcaster.CatchSignals();
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
        LOG(ERROR) << "Unhandled Exception reached the top of main: "
                   << e.what() << ", application will now exit";
        return 1;
    }

    return 0;
}

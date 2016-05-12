/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runExample2Sink.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include "FairMQLogger.h"
#include "FairMQDDSTools.h"
#include "FairMQProgOptions.h"
#include "FairMQExample3Sink.h"

int main(int argc, char** argv)
{
    try
    {
        FairMQProgOptions config;
        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        FairMQExample3Sink sink;
        sink.CatchSignals();
        sink.SetConfig(config);

        sink.ChangeState("INIT_DEVICE");
        HandleConfigViaDDS(sink);
        sink.WaitForEndOfState("INIT_DEVICE");

        sink.ChangeState("INIT_TASK");
        sink.WaitForEndOfState("INIT_TASK");

        runDDSStateHandler(sink);
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Unhandled Exception reached the top of main: "
                   << e.what() << ", application will now exit";
        return 1;
    }

    return 0;
}

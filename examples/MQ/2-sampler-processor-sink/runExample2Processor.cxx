/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runExample2Processor.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include "FairMQLogger.h"

#include "FairMQProgOptions.h"
#include "FairMQExample2Processor.h"

int main(int argc, char** argv)
{
    try
    {
        FairMQProgOptions config;
        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        FairMQExample2Processor processor;
        processor.CatchSignals();
        processor.SetConfig(config);

        processor.ChangeState("INIT_DEVICE");
        processor.WaitForEndOfState("INIT_DEVICE");

        processor.ChangeState("INIT_TASK");
        processor.WaitForEndOfState("INIT_TASK");

        processor.ChangeState("RUN");
        processor.InteractiveStateLoop();
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Unhandled Exception reached the top of main: "
                   << e.what() << ", application will now exit";
        return 1;
    }

    return 0;
}

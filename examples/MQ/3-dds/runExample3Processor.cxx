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
#include "FairMQDDSTools.h"
#include "FairMQProgOptions.h"
#include "FairMQExample3Processor.h"

using namespace std;
using namespace boost::program_options;

int main(int argc, char** argv)
{
    FairMQExample3Processor processor;
    processor.CatchSignals();

    FairMQProgOptions config;

    try
    {
        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        processor.SetConfig(config);

        processor.ChangeState("INIT_DEVICE");
        HandleConfigViaDDS(processor);
        processor.WaitForEndOfState("INIT_DEVICE");

        processor.ChangeState("INIT_TASK");
        processor.WaitForEndOfState("INIT_TASK");

        runDDSStateHandler(processor);
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

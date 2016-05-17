/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runMerger.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQProgOptions.h"
#include "FairMQMerger.h"
#include "runSimpleMQStateMachine.h"

using namespace boost::program_options;

int main(int argc, char** argv)
{
    try
    {
        int multipart;

        options_description mergerOptions("Proxy options");
        mergerOptions.add_options()
            ("multipart", value<int>(&multipart)->default_value(1), "Handle multipart payloads");

        FairMQProgOptions config;
        config.AddToCmdLineOptions(mergerOptions);
        config.ParseAll(argc, argv);

        FairMQMerger merger;
        merger.SetProperty(FairMQMerger::Multipart, multipart);
        runStateMachine(merger, config);
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Unhandled Exception reached the top of main: "
                   << e.what() << ", application will now exit";
        return 1;
    }

    return 0;
}

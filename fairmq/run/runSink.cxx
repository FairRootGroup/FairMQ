/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runSink.cxx
 *
 * @since 2013-01-21
 * @author: D. Klein, A. Rybalchenko
 */

#include <iostream>

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQProgOptions.h"
#include "FairMQSink.h"
#include "runSimpleMQStateMachine.h"

using namespace boost::program_options;

int main(int argc, char** argv)
{
    try
    {
        int numMsgs;

        options_description sinkOptions("Sink options");
        sinkOptions.add_options()
            ("num-msgs", value<int>(&numMsgs)->default_value(0), "Number of messages to receive");

        FairMQProgOptions config;
        config.AddToCmdLineOptions(sinkOptions);
        config.ParseAll(argc, argv);

        FairMQSink sink;
        sink.SetProperty(FairMQSink::NumMsgs, numMsgs);

        runStateMachine(sink, config);
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Unhandled Exception reached the top of main: "
                   << e.what() << ", application will now exit";
        return 1;
    }

    return 0;
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runBenchmarkSampler.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQProgOptions.h"
#include "FairMQBenchmarkSampler.h"
#include "runSimpleMQStateMachine.h"

using namespace boost::program_options;

int main(int argc, char** argv)
{
    try
    {
        int msgSize;
        int numMsgs;

        options_description samplerOptions("Sampler options");
        samplerOptions.add_options()
            ("msg-size", value<int>(&msgSize)->default_value(1000), "Message size in bytes")
            ("num-msgs", value<int>(&numMsgs)->default_value(0),    "Number of messages to send");

        FairMQProgOptions config;
        config.AddToCmdLineOptions(samplerOptions);

        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        FairMQBenchmarkSampler sampler;
        sampler.SetProperty(FairMQBenchmarkSampler::MsgSize, msgSize);
        sampler.SetProperty(FairMQBenchmarkSampler::NumMsgs, numMsgs);

        runStateMachine(sampler, config);
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Unhandled Exception reached the top of main: "
                   << e.what() << ", application will now exit";
        return 1;
    }

    return 0;
}

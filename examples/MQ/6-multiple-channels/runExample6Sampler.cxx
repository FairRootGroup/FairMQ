/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runExample6Sampler.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQProgOptions.h"
#include "FairMQExample6Sampler.h"

using namespace boost::program_options;

int main(int argc, char** argv)
{
    try
    {
        std::string text;

        options_description samplerOptions("Sampler options");
        samplerOptions.add_options()
            ("text", value<std::string>(&text)->default_value("Hello"), "Text to send out");

        FairMQProgOptions config;
        config.AddToCmdLineOptions(samplerOptions);
        config.ParseAll(argc, argv);

        FairMQExample6Sampler sampler;
        sampler.CatchSignals();
        sampler.SetConfig(config);
        sampler.SetProperty(FairMQExample6Sampler::Text, text);

        sampler.ChangeState("INIT_DEVICE");
        sampler.WaitForEndOfState("INIT_DEVICE");

        sampler.ChangeState("INIT_TASK");
        sampler.WaitForEndOfState("INIT_TASK");

        sampler.ChangeState("RUN");
        sampler.InteractiveStateLoop();
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Unhandled Exception reached the top of main: "
                   << e.what() << ", application will now exit";
        return 1;
    }

    return 0;
}

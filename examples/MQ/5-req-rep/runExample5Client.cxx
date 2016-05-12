/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runExampleClient.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQProgOptions.h"
#include "FairMQExample5Client.h"

using namespace boost::program_options;

int main(int argc, char** argv)
{
    try
    {
        std::string text;

        options_description clientOptions("Client options");
        clientOptions.add_options()
            ("text", value<std::string>(&text)->default_value("Hello"), "Text to send out");

        FairMQProgOptions config;
        config.AddToCmdLineOptions(clientOptions);
        config.ParseAll(argc, argv);

        FairMQExample5Client client;
        client.CatchSignals();
        client.SetConfig(config);
        client.SetProperty(FairMQExample5Client::Text, text);

        client.ChangeState("INIT_DEVICE");
        client.WaitForEndOfState("INIT_DEVICE");

        client.ChangeState("INIT_TASK");
        client.WaitForEndOfState("INIT_TASK");

        client.ChangeState("RUN");
        client.InteractiveStateLoop();
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Unhandled Exception reached the top of main: "
                   << e.what() << ", application will now exit";
        return 1;
    }

    return 0;
}

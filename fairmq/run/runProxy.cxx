/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runProxy.cxx
 *
 * @since 2013-10-07
 * @author A. Rybalchenko
 */

#include <iostream>

#include "FairMQLogger.h"
#include "FairMQProgOptions.h"
#include "FairMQProxy.h"
#include "runSimpleMQStateMachine.h"

int main(int argc, char** argv)
{
    try
    {
        FairMQProgOptions config;

        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        FairMQProxy proxy;
        runStateMachine(proxy, config);
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Unhandled Exception reached the top of main: "
                   << e.what() << ", application will now exit";
        return 1;
    }

    return 0;
}

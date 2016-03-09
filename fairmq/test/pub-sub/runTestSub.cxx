/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runTestSub.cxx
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include <string>

#include "FairMQLogger.h"
#include "FairMQTestSub.h"

int main(int /*argc*/, char** /*argv*/)
{
    FairMQTestSub testSub;
    testSub.CatchSignals();
    testSub.SetTransport("zeromq");

    testSub.SetProperty(FairMQTestSub::Id, "testSub_" + std::to_string(getpid()));

    FairMQChannel controlChannel("push", "connect", "tcp://127.0.0.1:5555");
    controlChannel.UpdateRateLogging(0);
    testSub.fChannels["control"].push_back(controlChannel);

    FairMQChannel subChannel("sub", "connect", "tcp://127.0.0.1:5556");
    subChannel.UpdateRateLogging(0);
    testSub.fChannels["data"].push_back(subChannel);

    testSub.ChangeState("INIT_DEVICE");
    testSub.WaitForEndOfState("INIT_DEVICE");

    testSub.ChangeState("INIT_TASK");
    testSub.WaitForEndOfState("INIT_TASK");

    testSub.ChangeState("RUN");
    testSub.WaitForEndOfState("RUN");

    testSub.ChangeState("RESET_TASK");
    testSub.WaitForEndOfState("RESET_TASK");

    testSub.ChangeState("RESET_DEVICE");
    testSub.WaitForEndOfState("RESET_DEVICE");

    testSub.ChangeState("END");

    return 0;
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runTestPull.cxx
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include "FairMQLogger.h"
#include "FairMQTestPull.h"

int main(int /*argc*/, char** /*argv*/)
{
    FairMQTestPull testPull;
    testPull.CatchSignals();
    testPull.SetTransport("zeromq");

    testPull.SetProperty(FairMQTestPull::Id, "testPull");

    FairMQChannel pullChannel("pull", "connect", "tcp://127.0.0.1:5557");
    testPull.fChannels["data"].push_back(pullChannel);

    testPull.ChangeState("INIT_DEVICE");
    testPull.WaitForEndOfState("INIT_DEVICE");

    testPull.ChangeState("INIT_TASK");
    testPull.WaitForEndOfState("INIT_TASK");

    testPull.ChangeState("RUN");
    testPull.WaitForEndOfState("RUN");

    testPull.ChangeState("RESET_TASK");
    testPull.WaitForEndOfState("RESET_TASK");

    testPull.ChangeState("RESET_DEVICE");
    testPull.WaitForEndOfState("RESET_DEVICE");

    testPull.ChangeState("END");

    return 0;
}

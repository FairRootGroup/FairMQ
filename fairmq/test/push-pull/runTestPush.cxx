/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runTestPush.cxx
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include "FairMQLogger.h"
#include "FairMQTestPush.h"

int main(int /*argc*/, char** /*argv*/)
{
    FairMQTestPush testPush;
    testPush.CatchSignals();
    testPush.SetTransport("zeromq");

    testPush.SetProperty(FairMQTestPush::Id, "testPush");

    FairMQChannel pushChannel("push", "bind", "tcp://127.0.0.1:5557");
    testPush.fChannels["data"].push_back(pushChannel);

    testPush.ChangeState("INIT_DEVICE");
    testPush.WaitForEndOfState("INIT_DEVICE");

    testPush.ChangeState("INIT_TASK");
    testPush.WaitForEndOfState("INIT_TASK");

    testPush.ChangeState("RUN");
    testPush.WaitForEndOfState("RUN");

    testPush.ChangeState("RESET_TASK");
    testPush.WaitForEndOfState("RESET_TASK");

    testPush.ChangeState("RESET_DEVICE");
    testPush.WaitForEndOfState("RESET_DEVICE");

    testPush.ChangeState("END");

    return 0;
}

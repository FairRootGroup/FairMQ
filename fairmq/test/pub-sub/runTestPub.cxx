/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runTestPub.cxx
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include "FairMQLogger.h"
#include "FairMQTestPub.h"

int main(int argc, char** argv)
{
    FairMQTestPub testPub;
    testPub.CatchSignals();
    if (argc == 2)
    {
        testPub.SetTransport(argv[1]);
    }
    else
    {
        testPub.SetTransport("zeromq");
    }

    reinit_logger(false);

    testPub.SetProperty(FairMQTestPub::Id, "testPub");

    FairMQChannel controlChannel("pull", "bind", "tcp://127.0.0.1:5555");
    controlChannel.UpdateRateLogging(0);
    testPub.fChannels["control"].push_back(controlChannel);

    FairMQChannel pubChannel("pub", "bind", "tcp://127.0.0.1:5556");
    pubChannel.UpdateRateLogging(0);
    testPub.fChannels["data"].push_back(pubChannel);

    testPub.ChangeState("INIT_DEVICE");
    testPub.WaitForEndOfState("INIT_DEVICE");

    testPub.ChangeState("INIT_TASK");
    testPub.WaitForEndOfState("INIT_TASK");

    testPub.ChangeState("RUN");
    testPub.WaitForEndOfState("RUN");

    testPub.ChangeState("RESET_TASK");
    testPub.WaitForEndOfState("RESET_TASK");

    testPub.ChangeState("RESET_DEVICE");
    testPub.WaitForEndOfState("RESET_DEVICE");

    testPub.ChangeState("END");

    return 0;
}

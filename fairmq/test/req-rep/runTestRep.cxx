/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runTestRep.cxx
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include "FairMQLogger.h"
#include "FairMQTestRep.h"

#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char** argv)
{
    reinit_logger(false);

    FairMQTestRep testRep;
    testRep.CatchSignals();

    std::string transport;
    if ( (argc != 2) || (argv[1] == NULL) )
    {
        LOG(ERROR) << "Transport for the test not specified!";
        return 1;
    }


    if ( strncmp(argv[1],"zeromq",6) == 0 )
    {
        transport = "zeromq";
        testRep.SetTransport(transport);
    }
    else if ( strncmp(argv[1],"nanomsg",7) == 0 )
    {
        transport = "nanomsg";
        testRep.SetTransport(transport);
    }
    else
    {
        LOG(ERROR) << "Incorrect transport requested! Expected 'zeromq' or 'nanomsg', found: " << argv[1];
        return 1;
    }

    testRep.SetProperty(FairMQTestRep::Id, "testRep");

    FairMQChannel repChannel("rep", "bind", "tcp://127.0.0.1:5558");
    if (transport == "nanomsg")
    {
        repChannel.UpdateAddress("tcp://127.0.0.1:5758");
    }
    repChannel.UpdateRateLogging(0);
    testRep.fChannels["data"].push_back(repChannel);

    testRep.ChangeState("INIT_DEVICE");
    testRep.WaitForEndOfState("INIT_DEVICE");

    testRep.ChangeState("INIT_TASK");
    testRep.WaitForEndOfState("INIT_TASK");

    testRep.ChangeState("RUN");
    testRep.WaitForEndOfState("RUN");

    // nanomsg does not implement the LINGER option. Give the sockets some time before their queues are terminated
    if (transport == "nanomsg")
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    testRep.ChangeState("RESET_TASK");
    testRep.WaitForEndOfState("RESET_TASK");

    testRep.ChangeState("RESET_DEVICE");
    testRep.WaitForEndOfState("RESET_DEVICE");

    testRep.ChangeState("END");

    return 0;
}

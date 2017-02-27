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

#include <chrono>
#include <thread>

int main(int argc, char** argv)
{
    reinit_logger(false);
    SET_LOG_CONSOLE_LEVEL(WARN);

    FairMQTestPush testPush;
    testPush.CatchSignals();

    std::string transport;
    if (argc != 2)
    {
        LOG(ERROR) << "Transport for the test not specified!";
        return 1;
    }
    transport = argv[1];

    if (transport == "zeromq" || transport == "nanomsg")
    {
        testPush.SetTransport(transport);
    }
    else
    {
        LOG(ERROR) << "Incorrect transport requested! Expected 'zeromq' or 'nanomsg', found: " << transport;
        return 1;
    }

    testPush.SetProperty(FairMQTestPush::Id, "testPush");

    FairMQChannel pushChannel("push", "bind", "tcp://127.0.0.1:5557");
    if (transport == "nanomsg")
    {
        pushChannel.UpdateAddress("tcp://127.0.0.1:5757");
    }
    pushChannel.UpdateRateLogging(0);
    testPush.fChannels["data"].push_back(pushChannel);

    testPush.ChangeState("INIT_DEVICE");
    testPush.WaitForEndOfState("INIT_DEVICE");

    testPush.ChangeState("INIT_TASK");
    testPush.WaitForEndOfState("INIT_TASK");

    testPush.ChangeState("RUN");
    testPush.WaitForEndOfState("RUN");

    // nanomsg does not implement the LINGER option. Give the sockets some time before their queues are terminated
    if (transport == "nanomsg")
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    testPush.ChangeState("RESET_TASK");
    testPush.WaitForEndOfState("RESET_TASK");

    testPush.ChangeState("RESET_DEVICE");
    testPush.WaitForEndOfState("RESET_DEVICE");

    testPush.ChangeState("END");

    return 0;
}

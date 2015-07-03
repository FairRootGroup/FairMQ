/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runExampleServer.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>

#include "FairMQLogger.h"
#include "FairMQExampleServer.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using namespace std;

int main(int argc, char** argv)
{
    FairMQExampleServer server;
    server.CatchSignals();

    LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

    server.SetTransport(transportFactory);

    server.SetProperty(FairMQExampleServer::Id, "server");
    server.SetProperty(FairMQExampleServer::NumIoThreads, 1);

    FairMQChannel replyChannel("rep", "bind", "tcp://*:5005");
    replyChannel.UpdateSndBufSize(10000);
    replyChannel.UpdateRcvBufSize(10000);
    replyChannel.UpdateRateLogging(1);

    server.fChannels["data"].push_back(replyChannel);

    server.ChangeState("INIT_DEVICE");
    server.WaitForEndOfState("INIT_DEVICE");

    server.ChangeState("INIT_TASK");
    server.WaitForEndOfState("INIT_TASK");

    server.ChangeState("RUN");
    server.InteractiveStateLoop();

    return 0;
}

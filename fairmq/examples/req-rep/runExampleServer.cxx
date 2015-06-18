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
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQExampleServer.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using namespace std;

FairMQExampleServer server;

static void s_signal_handler(int signal)
{
    LOG(INFO) << "Caught signal " << signal;

    server.ChangeState(FairMQExampleServer::END);

    LOG(INFO) << "Caught signal " << signal;
    exit(1);
}

static void s_catch_signals(void)
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

int main(int argc, char** argv)
{
    s_catch_signals();

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

    server.ChangeState(FairMQExampleServer::INIT_DEVICE);
    server.WaitForEndOfState(FairMQExampleServer::INIT_DEVICE);

    server.ChangeState(FairMQExampleServer::INIT_TASK);
    server.WaitForEndOfState(FairMQExampleServer::INIT_TASK);

    server.ChangeState(FairMQExampleServer::RUN);
    server.WaitForEndOfState(FairMQExampleServer::RUN);

    server.ChangeState(FairMQExampleServer::STOP);

    server.ChangeState(FairMQExampleServer::RESET_TASK);
    server.WaitForEndOfState(FairMQExampleServer::RESET_TASK);

    server.ChangeState(FairMQExampleServer::RESET_DEVICE);
    server.WaitForEndOfState(FairMQExampleServer::RESET_DEVICE);

    server.ChangeState(FairMQExampleServer::END);

    return 0;
}

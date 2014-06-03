/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runSink.cxx
 *
 * @since 2013-01-21
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQProtoSink.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using std::cout;
using std::cin;
using std::endl;
using std::stringstream;

FairMQProtoSink sink;

static void s_signal_handler(int signal)
{
    cout << endl << "Caught signal " << signal << endl;

    sink.ChangeState(FairMQProtoSink::STOP);
    sink.ChangeState(FairMQProtoSink::END);

    cout << "Shutdown complete. Bye!" << endl;
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
    if (argc != 7)
    {
        cout << "Usage: sink \tID numIoTreads\n"
             << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n" << endl;
        return 1;
    }

    s_catch_signals();

    LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

    sink.SetTransport(transportFactory);

    int i = 1;

    sink.SetProperty(FairMQProtoSink::Id, argv[i]);
    ++i;

    int numIoThreads;
    stringstream(argv[i]) >> numIoThreads;
    sink.SetProperty(FairMQProtoSink::NumIoThreads, numIoThreads);
    ++i;

    sink.SetProperty(FairMQProtoSink::NumInputs, 1);
    sink.SetProperty(FairMQProtoSink::NumOutputs, 0);

    sink.ChangeState(FairMQProtoSink::INIT);

    sink.SetProperty(FairMQProtoSink::InputSocketType, argv[i], 0);
    ++i;
    int inputRcvBufSize;
    stringstream(argv[i]) >> inputRcvBufSize;
    sink.SetProperty(FairMQProtoSink::InputRcvBufSize, inputRcvBufSize, 0);
    ++i;
    sink.SetProperty(FairMQProtoSink::InputMethod, argv[i], 0);
    ++i;
    sink.SetProperty(FairMQProtoSink::InputAddress, argv[i], 0);
    ++i;

    sink.ChangeState(FairMQProtoSink::SETOUTPUT);
    sink.ChangeState(FairMQProtoSink::SETINPUT);
    sink.ChangeState(FairMQProtoSink::RUN);

    char ch;
    cin.get(ch);

    sink.ChangeState(FairMQProtoSink::STOP);
    sink.ChangeState(FairMQProtoSink::END);

    return 0;
}

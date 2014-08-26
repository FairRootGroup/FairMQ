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
#include "FairMQSink.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using std::cout;
using std::cin;
using std::endl;
using std::stringstream;

FairMQSink sink;

static void s_signal_handler(int signal)
{
    cout << endl << "Caught signal " << signal << endl;

    sink.ChangeState(FairMQSink::STOP);
    sink.ChangeState(FairMQSink::END);

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

    sink.SetProperty(FairMQSink::Id, argv[i]);
    ++i;

    int numIoThreads;
    stringstream(argv[i]) >> numIoThreads;
    sink.SetProperty(FairMQSink::NumIoThreads, numIoThreads);
    ++i;

    sink.SetProperty(FairMQSink::NumInputs, 1);
    sink.SetProperty(FairMQSink::NumOutputs, 0);

    sink.ChangeState(FairMQSink::INIT);

    sink.SetProperty(FairMQSink::InputSocketType, argv[i], 0);
    ++i;
    int inputRcvBufSize;
    stringstream(argv[i]) >> inputRcvBufSize;
    sink.SetProperty(FairMQSink::InputRcvBufSize, inputRcvBufSize, 0);
    ++i;
    sink.SetProperty(FairMQSink::InputMethod, argv[i], 0);
    ++i;
    sink.SetProperty(FairMQSink::InputAddress, argv[i], 0);
    ++i;

    sink.ChangeState(FairMQSink::SETOUTPUT);
    sink.ChangeState(FairMQSink::SETINPUT);
    sink.ChangeState(FairMQSink::RUN);

    // wait until the running thread has finished processing.
    boost::unique_lock<boost::mutex> lock(sink.fRunningMutex);
    while (!sink.fRunningFinished)
    {
        sink.fRunningCondition.wait(lock);
    }

    sink.ChangeState(FairMQSink::STOP);
    sink.ChangeState(FairMQSink::END);

    return 0;
}

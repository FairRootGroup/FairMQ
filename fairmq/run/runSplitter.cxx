/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runSplitter.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQSplitter.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using std::cout;
using std::cin;
using std::endl;
using std::stringstream;

FairMQSplitter splitter;

static void s_signal_handler(int signal)
{
    cout << endl << "Caught signal " << signal << endl;

    splitter.ChangeState(FairMQSplitter::STOP);
    splitter.ChangeState(FairMQSplitter::END);

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
    if (argc < 16 || (argc - 8) % 4 != 0)
    {
        cout << "Usage: splitter \tID numIoTreads numOutputs\n"
             << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
             << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n"
             << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n"
             << "\t\t..." << argc << " arguments provided" << endl;
        return 1;
    }

    s_catch_signals();

    LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

    splitter.SetTransport(transportFactory);

    int i = 1;

    splitter.SetProperty(FairMQSplitter::Id, argv[i]);
    ++i;

    int numIoThreads;
    stringstream(argv[i]) >> numIoThreads;
    splitter.SetProperty(FairMQSplitter::NumIoThreads, numIoThreads);
    ++i;

    splitter.SetProperty(FairMQSplitter::NumInputs, 1);

    int numOutputs;
    stringstream(argv[i]) >> numOutputs;
    splitter.SetProperty(FairMQSplitter::NumOutputs, numOutputs);
    ++i;

    splitter.ChangeState(FairMQSplitter::INIT);

    splitter.SetProperty(FairMQSplitter::InputSocketType, argv[i], 0);
    ++i;
    int inputRcvBufSize;
    stringstream(argv[i]) >> inputRcvBufSize;
    splitter.SetProperty(FairMQSplitter::InputRcvBufSize, inputRcvBufSize, 0);
    ++i;
    splitter.SetProperty(FairMQSplitter::InputMethod, argv[i], 0);
    ++i;
    splitter.SetProperty(FairMQSplitter::InputAddress, argv[i], 0);
    ++i;

    int outputSndBufSize;
    for (int iOutput = 0; iOutput < numOutputs; iOutput++)
    {
        splitter.SetProperty(FairMQSplitter::OutputSocketType, argv[i], iOutput);
        ++i;
        stringstream(argv[i]) >> outputSndBufSize;
        splitter.SetProperty(FairMQSplitter::OutputSndBufSize, outputSndBufSize, iOutput);
        ++i;
        splitter.SetProperty(FairMQSplitter::OutputMethod, argv[i], iOutput);
        ++i;
        splitter.SetProperty(FairMQSplitter::OutputAddress, argv[i], iOutput);
        ++i;
    }

    splitter.ChangeState(FairMQSplitter::SETOUTPUT);
    splitter.ChangeState(FairMQSplitter::SETINPUT);
    splitter.ChangeState(FairMQSplitter::RUN);

    // wait until the running thread has finished processing.
    boost::unique_lock<boost::mutex> lock(splitter.fRunningMutex);
    while (!splitter.fRunningFinished)
    {
        splitter.fRunningCondition.wait(lock);
    }

    splitter.ChangeState(FairMQSplitter::STOP);
    splitter.ChangeState(FairMQSplitter::END);

    return 0;
}

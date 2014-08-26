/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runBenchmarkSampler.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQBinSampler.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using std::cout;
using std::cin;
using std::endl;
using std::stringstream;

FairMQBinSampler sampler;

static void s_signal_handler(int signal)
{
    cout << endl << "Caught signal " << signal << endl;

    sampler.ChangeState(FairMQBinSampler::STOP);
    sampler.ChangeState(FairMQBinSampler::END);

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
    if (argc != 9)
    {
        cout << "Usage: bsampler ID eventSize eventRate numIoTreads\n"
             << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n" << endl;
        return 1;
    }

    s_catch_signals();

    LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

    sampler.SetTransport(transportFactory);

    int i = 1;

    sampler.SetProperty(FairMQBinSampler::Id, argv[i]);
    ++i;

    int eventSize;
    stringstream(argv[i]) >> eventSize;
    sampler.SetProperty(FairMQBinSampler::EventSize, eventSize);
    ++i;

    int eventRate;
    stringstream(argv[i]) >> eventRate;
    sampler.SetProperty(FairMQBinSampler::EventRate, eventRate);
    ++i;

    int numIoThreads;
    stringstream(argv[i]) >> numIoThreads;
    sampler.SetProperty(FairMQBinSampler::NumIoThreads, numIoThreads);
    ++i;

    sampler.SetProperty(FairMQBinSampler::NumInputs, 0);
    sampler.SetProperty(FairMQBinSampler::NumOutputs, 1);

    sampler.ChangeState(FairMQBinSampler::INIT);

    sampler.SetProperty(FairMQBinSampler::OutputSocketType, argv[i], 0);
    ++i;
    int outputSndBufSize;
    stringstream(argv[i]) >> outputSndBufSize;
    sampler.SetProperty(FairMQBinSampler::OutputSndBufSize, outputSndBufSize, 0);
    ++i;
    sampler.SetProperty(FairMQBinSampler::OutputMethod, argv[i], 0);
    ++i;
    sampler.SetProperty(FairMQBinSampler::OutputAddress, argv[i], 0);
    ++i;

    sampler.ChangeState(FairMQBinSampler::SETOUTPUT);
    sampler.ChangeState(FairMQBinSampler::SETINPUT);
    sampler.ChangeState(FairMQBinSampler::RUN);

    // wait until the running thread has finished processing.
    boost::unique_lock<boost::mutex> lock(sampler.fRunningMutex);
    while (!sampler.fRunningFinished)
    {
        sampler.fRunningCondition.wait(lock);
    }

    sampler.ChangeState(FairMQBinSampler::STOP);
    sampler.ChangeState(FairMQBinSampler::END);

    return 0;
}

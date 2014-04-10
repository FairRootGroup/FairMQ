/**
 * runBenchmarkSampler.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQProtoSampler.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using std::cout;
using std::cin;
using std::endl;
using std::stringstream;

FairMQProtoSampler sampler;

static void s_signal_handler(int signal)
{
    cout << endl << "Caught signal " << signal << endl;

    sampler.ChangeState(FairMQProtoSampler::STOP);
    sampler.ChangeState(FairMQProtoSampler::END);

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

    sampler.SetProperty(FairMQProtoSampler::Id, argv[i]);
    ++i;

    int eventSize;
    stringstream(argv[i]) >> eventSize;
    sampler.SetProperty(FairMQProtoSampler::EventSize, eventSize);
    ++i;

    int eventRate;
    stringstream(argv[i]) >> eventRate;
    sampler.SetProperty(FairMQProtoSampler::EventRate, eventRate);
    ++i;

    int numIoThreads;
    stringstream(argv[i]) >> numIoThreads;
    sampler.SetProperty(FairMQProtoSampler::NumIoThreads, numIoThreads);
    ++i;

    sampler.SetProperty(FairMQProtoSampler::NumInputs, 0);
    sampler.SetProperty(FairMQProtoSampler::NumOutputs, 1);

    sampler.ChangeState(FairMQProtoSampler::INIT);

    sampler.SetProperty(FairMQProtoSampler::OutputSocketType, argv[i], 0);
    ++i;
    int outputSndBufSize;
    stringstream(argv[i]) >> outputSndBufSize;
    sampler.SetProperty(FairMQProtoSampler::OutputSndBufSize, outputSndBufSize, 0);
    ++i;
    sampler.SetProperty(FairMQProtoSampler::OutputMethod, argv[i], 0);
    ++i;
    sampler.SetProperty(FairMQProtoSampler::OutputAddress, argv[i], 0);
    ++i;

    sampler.ChangeState(FairMQProtoSampler::SETOUTPUT);
    sampler.ChangeState(FairMQProtoSampler::SETINPUT);
    sampler.ChangeState(FairMQProtoSampler::RUN);

    char ch;
    cin.get(ch);

    sampler.ChangeState(FairMQProtoSampler::STOP);
    sampler.ChangeState(FairMQProtoSampler::END);

    return 0;
}

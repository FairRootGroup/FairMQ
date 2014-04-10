/**
 * runBuffer.cxx
 *
 * @since 2012-10-26
 * @author: D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQBuffer.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using std::cout;
using std::cin;
using std::endl;
using std::stringstream;

FairMQBuffer buffer;

static void s_signal_handler(int signal)
{
    cout << endl << "Caught signal " << signal << endl;

    buffer.ChangeState(FairMQBuffer::STOP);
    buffer.ChangeState(FairMQBuffer::END);

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
    if (argc != 11)
    {
        cout << "Usage: buffer \tID numIoTreads\n"
             << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
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

    buffer.SetTransport(transportFactory);

    int i = 1;

    buffer.SetProperty(FairMQBuffer::Id, argv[i]);
    ++i;

    int numIoThreads;
    stringstream(argv[i]) >> numIoThreads;
    buffer.SetProperty(FairMQBuffer::NumIoThreads, numIoThreads);
    ++i;
    buffer.SetProperty(FairMQBuffer::NumInputs, 1);
    buffer.SetProperty(FairMQBuffer::NumOutputs, 1);

    buffer.ChangeState(FairMQBuffer::INIT);

    buffer.SetProperty(FairMQBuffer::InputSocketType, argv[i], 0);
    ++i;
    int inputRcvBufSize;
    stringstream(argv[i]) >> inputRcvBufSize;
    buffer.SetProperty(FairMQBuffer::InputRcvBufSize, inputRcvBufSize, 0);
    ++i;
    buffer.SetProperty(FairMQBuffer::InputMethod, argv[i], 0);
    ++i;
    buffer.SetProperty(FairMQBuffer::InputAddress, argv[i], 0);
    ++i;

    buffer.SetProperty(FairMQBuffer::OutputSocketType, argv[i], 0);
    ++i;
    int outputSndBufSize;
    stringstream(argv[i]) >> outputSndBufSize;
    buffer.SetProperty(FairMQBuffer::OutputSndBufSize, outputSndBufSize, 0);
    ++i;
    buffer.SetProperty(FairMQBuffer::OutputMethod, argv[i], 0);
    ++i;
    buffer.SetProperty(FairMQBuffer::OutputAddress, argv[i], 0);
    ++i;

    buffer.ChangeState(FairMQBuffer::SETOUTPUT);
    buffer.ChangeState(FairMQBuffer::SETINPUT);
    buffer.ChangeState(FairMQBuffer::RUN);

    char ch;
    cin.get(ch);

    buffer.ChangeState(FairMQBuffer::STOP);
    buffer.ChangeState(FairMQBuffer::END);

    return 0;
}

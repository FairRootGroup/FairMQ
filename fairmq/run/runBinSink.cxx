/**
 * runSink.cxx
 *
 * @since 2013-01-21
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQBinSink.h"

#ifdef NANOMSG
  #include "FairMQTransportFactoryNN.h"
#else
  #include "FairMQTransportFactoryZMQ.h"
#endif

using std::cout;
using std::cin;
using std::endl;
using std::stringstream;


FairMQBinSink sink;

static void s_signal_handler (int signal)
{
  cout << endl << "Caught signal " << signal << endl;

  sink.ChangeState(FairMQBinSink::STOP);
  sink.ChangeState(FairMQBinSink::END);

  cout << "Shutdown complete. Bye!" << endl;
  exit(1);
}

static void s_catch_signals (void)
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
  if ( argc != 7 ) {
    cout << "Usage: sink \tID numIoTreads\n"
              << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
              << endl;
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

  sink.SetProperty(FairMQBinSink::Id, argv[i]);
  ++i;

  int numIoThreads;
  stringstream(argv[i]) >> numIoThreads;
  sink.SetProperty(FairMQBinSink::NumIoThreads, numIoThreads);
  ++i;

  sink.SetProperty(FairMQBinSink::NumInputs, 1);
  sink.SetProperty(FairMQBinSink::NumOutputs, 0);


  sink.ChangeState(FairMQBinSink::INIT);


  sink.SetProperty(FairMQBinSink::InputSocketType, argv[i], 0);
  ++i;
  int inputRcvBufSize;
  stringstream(argv[i]) >> inputRcvBufSize;
  sink.SetProperty(FairMQBinSink::InputRcvBufSize, inputRcvBufSize, 0);
  ++i;
  sink.SetProperty(FairMQBinSink::InputMethod, argv[i], 0);
  ++i;
  sink.SetProperty(FairMQBinSink::InputAddress, argv[i], 0);
  ++i;


  sink.ChangeState(FairMQBinSink::SETOUTPUT);
  sink.ChangeState(FairMQBinSink::SETINPUT);
  sink.ChangeState(FairMQBinSink::RUN);


  char ch;
  cin.get(ch);

  sink.ChangeState(FairMQBinSink::STOP);
  sink.ChangeState(FairMQBinSink::END);

  return 0;
}


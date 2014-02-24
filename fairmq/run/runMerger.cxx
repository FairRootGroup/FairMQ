/**
 * runMerger.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQMerger.h"

#ifdef NANOMSG
  #include "FairMQTransportFactoryNN.h"
#else
  #include "FairMQTransportFactoryZMQ.h"
#endif

using std::cout;
using std::cin;
using std::endl;
using std::stringstream;


FairMQMerger merger;

static void s_signal_handler (int signal)
{
  cout << endl << "Caught signal " << signal << endl;

  merger.ChangeState(FairMQMerger::STOP);
  merger.ChangeState(FairMQMerger::END);

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
  if ( argc < 16 || (argc - 8) % 4 != 0 ) {
    cout << "Usage: merger \tID numIoTreads numInputs\n"
              << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
              << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
              << "\t\t...\n"
              << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n"
              << argc << " arguments provided" << endl;
    return 1;
  }

  s_catch_signals();

  LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
  FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
  FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

  merger.SetTransport(transportFactory);

  int i = 1;

  merger.SetProperty(FairMQMerger::Id, argv[i]);
  ++i;

  int numIoThreads;
  stringstream(argv[i]) >> numIoThreads;
  merger.SetProperty(FairMQMerger::NumIoThreads, numIoThreads);
  ++i;

  int numInputs;
  stringstream(argv[i]) >> numInputs;
  merger.SetProperty(FairMQMerger::NumInputs, numInputs);
  ++i;

  merger.SetProperty(FairMQMerger::NumOutputs, 1);

  merger.ChangeState(FairMQMerger::INIT);

  for (int iInput = 0; iInput < numInputs; iInput++ ) {
    merger.SetProperty(FairMQMerger::InputSocketType, argv[i], iInput);
    ++i;
    int inputRcvBufSize;
    stringstream(argv[i]) >> inputRcvBufSize;
    merger.SetProperty(FairMQMerger::InputRcvBufSize, inputRcvBufSize, iInput);
    ++i;
    merger.SetProperty(FairMQMerger::InputMethod, argv[i], iInput);
    ++i;
    merger.SetProperty(FairMQMerger::InputAddress, argv[i], iInput);
    ++i;
  }

  merger.SetProperty(FairMQMerger::OutputSocketType, argv[i], 0);
  ++i;
  int outputSndBufSize;
  stringstream(argv[i]) >> outputSndBufSize;
  merger.SetProperty(FairMQMerger::OutputSndBufSize, outputSndBufSize, 0);
  ++i;
  merger.SetProperty(FairMQMerger::OutputMethod, argv[i], 0);
  ++i;
  merger.SetProperty(FairMQMerger::OutputAddress, argv[i], 0);
  ++i;

  merger.ChangeState(FairMQMerger::SETOUTPUT);
  merger.ChangeState(FairMQMerger::SETINPUT);
  merger.ChangeState(FairMQMerger::RUN);


  char ch;
  cin.get(ch);

  merger.ChangeState(FairMQMerger::STOP);
  merger.ChangeState(FairMQMerger::END);

  return 0;
}


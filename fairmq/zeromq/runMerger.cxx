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
#include "FairMQTransportFactoryZMQ.h"

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
  if ( argc != 15 ) {
    cout << "Usage: merger \tID numIoTreads\n"
              << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
              << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
              << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n" << endl;
    return 1;
  }

  s_catch_signals();

  stringstream logmsg;
  logmsg << "PID: " << getpid();
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
  merger.SetTransport(transportFactory);

  int i = 1;

  merger.SetProperty(FairMQMerger::Id, argv[i]);
  ++i;

  int numIoThreads;
  stringstream(argv[i]) >> numIoThreads;
  merger.SetProperty(FairMQMerger::NumIoThreads, numIoThreads);
  ++i;

  merger.SetProperty(FairMQMerger::NumInputs, 2);
  merger.SetProperty(FairMQMerger::NumOutputs, 1);


  merger.ChangeState(FairMQMerger::INIT);


  merger.SetProperty(FairMQMerger::InputSocketType, argv[i], 0);
  ++i;
  int inputRcvBufSize;
  stringstream(argv[i]) >> inputRcvBufSize;
  merger.SetProperty(FairMQMerger::InputRcvBufSize, inputRcvBufSize, 0);
  ++i;
  merger.SetProperty(FairMQMerger::InputMethod, argv[i], 0);
  ++i;
  merger.SetProperty(FairMQMerger::InputAddress, argv[i], 0);
  ++i;

  merger.SetProperty(FairMQMerger::InputSocketType, argv[i], 1);
  ++i;
  stringstream(argv[i]) >> inputRcvBufSize;
  merger.SetProperty(FairMQMerger::InputRcvBufSize, inputRcvBufSize, 1);
  ++i;
  merger.SetProperty(FairMQMerger::InputMethod, argv[i], 1);
  ++i;
  merger.SetProperty(FairMQMerger::InputAddress, argv[i], 1);
  ++i;

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


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


FairMQMerger merger;

static void s_signal_handler (int signal)
{
  std::cout << std::endl << "Caught signal " << signal << std::endl;

  merger.ChangeState(FairMQMerger::STOP);
  merger.ChangeState(FairMQMerger::END);

  std::cout << "Shutdown complete. Bye!" << std::endl;
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
    std::cout << "Usage: merger \tID numIoTreads\n"
              << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
              << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
              << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n" << std::endl;
    return 1;
  }

  s_catch_signals();

  std::stringstream logmsg;
  logmsg << "PID: " << getpid();
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  int i = 1;

  merger.SetProperty(FairMQMerger::Id, argv[i]);
  ++i;

  int numIoThreads;
  std::stringstream(argv[i]) >> numIoThreads;
  merger.SetProperty(FairMQMerger::NumIoThreads, numIoThreads);
  ++i;

  merger.SetProperty(FairMQMerger::NumInputs, 2);
  merger.SetProperty(FairMQMerger::NumOutputs, 1);


  merger.ChangeState(FairMQMerger::INIT);


  int inputSocketType = ZMQ_SUB;
  if (strcmp(argv[i], "pull") == 0) {
    inputSocketType = ZMQ_PULL;
  }
  merger.SetProperty(FairMQMerger::InputSocketType, inputSocketType, 0);
  ++i;
  int inputRcvBufSize;
  std::stringstream(argv[i]) >> inputRcvBufSize;
  merger.SetProperty(FairMQMerger::InputRcvBufSize, inputRcvBufSize, 0);
  ++i;
  merger.SetProperty(FairMQMerger::InputMethod, argv[i], 0);
  ++i;
  merger.SetProperty(FairMQMerger::InputAddress, argv[i], 0);
  ++i;

  inputSocketType = ZMQ_SUB;
  if (strcmp(argv[i], "pull") == 0) {
    inputSocketType = ZMQ_PULL;
  }
  merger.SetProperty(FairMQMerger::InputSocketType, inputSocketType, 1);
  ++i;
  std::stringstream(argv[i]) >> inputRcvBufSize;
  merger.SetProperty(FairMQMerger::InputRcvBufSize, inputRcvBufSize, 1);
  ++i;
  merger.SetProperty(FairMQMerger::InputMethod, argv[i], 1);
  ++i;
  merger.SetProperty(FairMQMerger::InputAddress, argv[i], 1);
  ++i;

  int outputSocketType = ZMQ_PUB;
  if (strcmp(argv[i], "push") == 0) {
    outputSocketType = ZMQ_PUSH;
  }
  merger.SetProperty(FairMQMerger::OutputSocketType, outputSocketType, 0);
  ++i;
  int outputSndBufSize;
  std::stringstream(argv[i]) >> outputSndBufSize;
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
  std::cin.get(ch);

  merger.ChangeState(FairMQMerger::STOP);
  merger.ChangeState(FairMQMerger::END);

  return 0;
}


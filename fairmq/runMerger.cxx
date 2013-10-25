/*
 * runMerger.cxx
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQStandaloneMerger.h"


FairMQStandaloneMerger merger;

static void s_signal_handler (int signal)
{
  std::cout << std::endl << "Caught signal " << signal << std::endl;

  merger.ChangeState(FairMQStandaloneMerger::STOP);
  merger.ChangeState(FairMQStandaloneMerger::END);

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

  merger.SetProperty(FairMQStandaloneMerger::Id, argv[i]);
  ++i;

  int numIoThreads;
  std::stringstream(argv[i]) >> numIoThreads;
  merger.SetProperty(FairMQStandaloneMerger::NumIoThreads, numIoThreads);
  ++i;

  merger.SetProperty(FairMQStandaloneMerger::NumInputs, 2);
  merger.SetProperty(FairMQStandaloneMerger::NumOutputs, 1);


  merger.ChangeState(FairMQStandaloneMerger::INIT);


  int inputSocketType = ZMQ_SUB;
  if (strcmp(argv[i], "pull") == 0) {
    inputSocketType = ZMQ_PULL;
  }
  merger.SetProperty(FairMQStandaloneMerger::InputSocketType, inputSocketType, 0);
  ++i;
  int inputRcvBufSize;
  std::stringstream(argv[i]) >> inputRcvBufSize;
  merger.SetProperty(FairMQStandaloneMerger::InputRcvBufSize, inputRcvBufSize, 0);
  ++i;
  merger.SetProperty(FairMQStandaloneMerger::InputMethod, argv[i], 0);
  ++i;
  merger.SetProperty(FairMQStandaloneMerger::InputAddress, argv[i], 0);
  ++i;

  inputSocketType = ZMQ_SUB;
  if (strcmp(argv[i], "pull") == 0) {
    inputSocketType = ZMQ_PULL;
  }
  merger.SetProperty(FairMQStandaloneMerger::InputSocketType, inputSocketType, 1);
  ++i;
  std::stringstream(argv[i]) >> inputRcvBufSize;
  merger.SetProperty(FairMQStandaloneMerger::InputRcvBufSize, inputRcvBufSize, 1);
  ++i;
  merger.SetProperty(FairMQStandaloneMerger::InputMethod, argv[i], 1);
  ++i;
  merger.SetProperty(FairMQStandaloneMerger::InputAddress, argv[i], 1);
  ++i;

  int outputSocketType = ZMQ_PUB;
  if (strcmp(argv[i], "push") == 0) {
    outputSocketType = ZMQ_PUSH;
  }
  merger.SetProperty(FairMQStandaloneMerger::OutputSocketType, outputSocketType, 0);
  ++i;
  int outputSndBufSize;
  std::stringstream(argv[i]) >> outputSndBufSize;
  merger.SetProperty(FairMQStandaloneMerger::OutputSndBufSize, outputSndBufSize, 0);
  ++i;
  merger.SetProperty(FairMQStandaloneMerger::OutputMethod, argv[i], 0);
  ++i;
  merger.SetProperty(FairMQStandaloneMerger::OutputAddress, argv[i], 0);
  ++i;


  merger.ChangeState(FairMQStandaloneMerger::SETOUTPUT);
  merger.ChangeState(FairMQStandaloneMerger::SETINPUT);
  merger.ChangeState(FairMQStandaloneMerger::RUN);


  char ch;
  std::cin.get(ch);

  merger.ChangeState(FairMQStandaloneMerger::STOP);
  merger.ChangeState(FairMQStandaloneMerger::END);

  return 0;
}


/*
 * runSplitter.cxx
 *
 *  Created on: Dec 6, 2012
 *      Author: dklein
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQBalancedStandaloneSplitter.h"


FairMQBalancedStandaloneSplitter splitter;

static void s_signal_handler (int signal)
{
  std::cout << std::endl << "Caught signal " << signal << std::endl;

  splitter.ChangeState(FairMQBalancedStandaloneSplitter::STOP);
  splitter.ChangeState(FairMQBalancedStandaloneSplitter::END);

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
    std::cout << "Usage: splitter \tID numIoTreads\n"
              << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
              << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n"
              << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n" << std::endl;
    return 1;
  }

  s_catch_signals();

  std::stringstream logmsg;
  logmsg << "PID: " << getpid();
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  int i = 1;

  splitter.SetProperty(FairMQBalancedStandaloneSplitter::Id, argv[i]);
  ++i;

  int numIoThreads;
  std::stringstream(argv[i]) >> numIoThreads;
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::NumIoThreads, numIoThreads);
  ++i;

  splitter.SetProperty(FairMQBalancedStandaloneSplitter::NumInputs, 1);
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::NumOutputs, 2);


  splitter.ChangeState(FairMQBalancedStandaloneSplitter::INIT);


  int inputSocketType = ZMQ_SUB;
  if (strcmp(argv[i], "pull") == 0) {
    inputSocketType = ZMQ_PULL;
  }
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::InputSocketType, inputSocketType, 0);
  ++i;
  int inputRcvBufSize;
  std::stringstream(argv[i]) >> inputRcvBufSize;
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::InputRcvBufSize, inputRcvBufSize, 0);
  ++i;
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::InputMethod, argv[i], 0);
  ++i;
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::InputAddress, argv[i], 0);
  ++i;

  int outputSocketType = ZMQ_PUB;
  if (strcmp(argv[i], "push") == 0) {
    outputSocketType = ZMQ_PUSH;
  }
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::OutputSocketType, outputSocketType, 0);
  ++i;
  int outputSndBufSize;
  std::stringstream(argv[i]) >> outputSndBufSize;
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::OutputSndBufSize, outputSndBufSize, 0);
  ++i;
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::OutputMethod, argv[i], 0);
  ++i;
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::OutputAddress, argv[i], 0);
  ++i;

  outputSocketType = ZMQ_PUB;
  if (strcmp(argv[i], "push") == 0) {
    outputSocketType = ZMQ_PUSH;
  }
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::OutputSocketType, outputSocketType, 1);
  ++i;
  std::stringstream(argv[i]) >> outputSndBufSize;
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::OutputSndBufSize, outputSndBufSize, 1);
  ++i;
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::OutputMethod, argv[i], 1);
  ++i;
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::OutputAddress, argv[i], 1);
  ++i;


  splitter.ChangeState(FairMQBalancedStandaloneSplitter::SETOUTPUT);
  splitter.ChangeState(FairMQBalancedStandaloneSplitter::SETINPUT);
  splitter.ChangeState(FairMQBalancedStandaloneSplitter::RUN);


  char ch;
  std::cin.get(ch);

  splitter.ChangeState(FairMQBalancedStandaloneSplitter::STOP);
  splitter.ChangeState(FairMQBalancedStandaloneSplitter::END);

  return 0;
}


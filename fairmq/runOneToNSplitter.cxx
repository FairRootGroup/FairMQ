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
  if ( argc < 16 || (argc-8)%4!=0 ) { //argc{name,id,threads,nout,insock,inbuff,inmet,inadd, ... out}
    std::cout << "Usage: splitter \tID numIoTreads numOutputs\n"
              << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
              << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n"
              << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n"
              << "\t\t..." << argc << " arguments provided" << std::endl;
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

  int numOutputs;
  std::stringstream(argv[i]) >> numOutputs;
  splitter.SetProperty(FairMQBalancedStandaloneSplitter::NumOutputs, numOutputs);
  ++i;


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

  int outputSocketType;
  int outputSndBufSize;
  for (int iOutput = 0; iOutput < numOutputs; iOutput++) {
    outputSocketType = ZMQ_PUB;
    if (strcmp(argv[i], "push") == 0) {
      outputSocketType = ZMQ_PUSH;
    }
    splitter.SetProperty(FairMQBalancedStandaloneSplitter::OutputSocketType, outputSocketType, iOutput);
    ++i;
    std::stringstream(argv[i]) >> outputSndBufSize;
    splitter.SetProperty(FairMQBalancedStandaloneSplitter::OutputSndBufSize, outputSndBufSize, iOutput);
    ++i;
    splitter.SetProperty(FairMQBalancedStandaloneSplitter::OutputMethod, argv[i], iOutput);
    ++i;
    splitter.SetProperty(FairMQBalancedStandaloneSplitter::OutputAddress, argv[i], iOutput);
    ++i;
  }

  splitter.ChangeState(FairMQBalancedStandaloneSplitter::SETOUTPUT);
  splitter.ChangeState(FairMQBalancedStandaloneSplitter::SETINPUT);
  splitter.ChangeState(FairMQBalancedStandaloneSplitter::RUN);


  char ch;
  std::cin.get(ch);

  splitter.ChangeState(FairMQBalancedStandaloneSplitter::STOP);
  splitter.ChangeState(FairMQBalancedStandaloneSplitter::END);

  return 0;
}


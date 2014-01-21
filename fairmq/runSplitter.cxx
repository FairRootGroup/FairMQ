/**
 * runSplitter.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQSplitter.h"
#include "FairMQTransportFactoryZMQ.h"


FairMQSplitter splitter;

static void s_signal_handler (int signal)
{
  std::cout << std::endl << "Caught signal " << signal << std::endl;

  splitter.ChangeState(FairMQSplitter::STOP);
  splitter.ChangeState(FairMQSplitter::END);

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

  FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
  splitter.SetTransport(transportFactory);

  int i = 1;

  splitter.SetProperty(FairMQSplitter::Id, argv[i]);
  ++i;

  int numIoThreads;
  std::stringstream(argv[i]) >> numIoThreads;
  splitter.SetProperty(FairMQSplitter::NumIoThreads, numIoThreads);
  ++i;

  splitter.SetProperty(FairMQSplitter::NumInputs, 1);
  splitter.SetProperty(FairMQSplitter::NumOutputs, 2);


  splitter.ChangeState(FairMQSplitter::INIT);


  int inputSocketType = ZMQ_SUB;
  if (strcmp(argv[i], "pull") == 0) {
    inputSocketType = ZMQ_PULL;
  }
  splitter.SetProperty(FairMQSplitter::InputSocketType, inputSocketType, 0);
  ++i;
  int inputRcvBufSize;
  std::stringstream(argv[i]) >> inputRcvBufSize;
  splitter.SetProperty(FairMQSplitter::InputRcvBufSize, inputRcvBufSize, 0);
  ++i;
  splitter.SetProperty(FairMQSplitter::InputMethod, argv[i], 0);
  ++i;
  splitter.SetProperty(FairMQSplitter::InputAddress, argv[i], 0);
  ++i;

  int outputSocketType = ZMQ_PUB;
  if (strcmp(argv[i], "push") == 0) {
    outputSocketType = ZMQ_PUSH;
  }
  splitter.SetProperty(FairMQSplitter::OutputSocketType, outputSocketType, 0);
  ++i;
  int outputSndBufSize;
  std::stringstream(argv[i]) >> outputSndBufSize;
  splitter.SetProperty(FairMQSplitter::OutputSndBufSize, outputSndBufSize, 0);
  ++i;
  splitter.SetProperty(FairMQSplitter::OutputMethod, argv[i], 0);
  ++i;
  splitter.SetProperty(FairMQSplitter::OutputAddress, argv[i], 0);
  ++i;

  outputSocketType = ZMQ_PUB;
  if (strcmp(argv[i], "push") == 0) {
    outputSocketType = ZMQ_PUSH;
  }
  splitter.SetProperty(FairMQSplitter::OutputSocketType, outputSocketType, 1);
  ++i;
  std::stringstream(argv[i]) >> outputSndBufSize;
  splitter.SetProperty(FairMQSplitter::OutputSndBufSize, outputSndBufSize, 1);
  ++i;
  splitter.SetProperty(FairMQSplitter::OutputMethod, argv[i], 1);
  ++i;
  splitter.SetProperty(FairMQSplitter::OutputAddress, argv[i], 1);
  ++i;


  splitter.ChangeState(FairMQSplitter::SETOUTPUT);
  splitter.ChangeState(FairMQSplitter::SETINPUT);
  splitter.ChangeState(FairMQSplitter::RUN);


  char ch;
  std::cin.get(ch);

  splitter.ChangeState(FairMQSplitter::STOP);
  splitter.ChangeState(FairMQSplitter::END);

  return 0;
}


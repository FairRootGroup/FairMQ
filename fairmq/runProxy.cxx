/**
 * runProxy.cxx
 *
 *  @since: Oct 07, 2013
 *  @authors: A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQProxy.h"


FairMQProxy proxy;

static void s_signal_handler (int signal)
{
  std::cout << std::endl << "Caught signal " << signal << std::endl;

  proxy.ChangeState(FairMQProxy::STOP);
  proxy.ChangeState(FairMQProxy::END);

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
  if ( argc != 11 ) {
    std::cout << "Usage: proxy \tID numIoTreads\n"
              << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
              << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n" << std::endl;
    return 1;
  }

  s_catch_signals();

  std::stringstream logmsg;
  logmsg << "PID: " << getpid();
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  int i = 1;

  proxy.SetProperty(FairMQProxy::Id, argv[i]);
  ++i;

  int numIoThreads;
  std::stringstream(argv[i]) >> numIoThreads;
  proxy.SetProperty(FairMQProxy::NumIoThreads, numIoThreads);
  ++i;

  proxy.SetProperty(FairMQProxy::NumInputs, 1);
  proxy.SetProperty(FairMQProxy::NumOutputs, 1);


  proxy.ChangeState(FairMQProxy::INIT);


  int inputSocketType = ZMQ_XSUB;
  if (strcmp(argv[i], "pull") == 0) {
    inputSocketType = ZMQ_PULL;
  }
  proxy.SetProperty(FairMQProxy::InputSocketType, inputSocketType, 0);
  ++i;
  int inputRcvBufSize;
  std::stringstream(argv[i]) >> inputRcvBufSize;
  proxy.SetProperty(FairMQProxy::InputRcvBufSize, inputRcvBufSize, 0);
  ++i;
  proxy.SetProperty(FairMQProxy::InputMethod, argv[i], 0);
  ++i;
  proxy.SetProperty(FairMQProxy::InputAddress, argv[i], 0);
  ++i;

  int outputSocketType = ZMQ_XPUB;
  if (strcmp(argv[i], "push") == 0) {
    outputSocketType = ZMQ_PUSH;
  }
  proxy.SetProperty(FairMQProxy::OutputSocketType, outputSocketType, 0);
  ++i;
  int outputSndBufSize;
  std::stringstream(argv[i]) >> outputSndBufSize;
  proxy.SetProperty(FairMQProxy::OutputSndBufSize, outputSndBufSize, 0);
  ++i;
  proxy.SetProperty(FairMQProxy::OutputMethod, argv[i], 0);
  ++i;
  proxy.SetProperty(FairMQProxy::OutputAddress, argv[i], 0);
  ++i;


  proxy.ChangeState(FairMQProxy::SETOUTPUT);
  proxy.ChangeState(FairMQProxy::SETINPUT);
  proxy.ChangeState(FairMQProxy::RUN);


  char ch;
  std::cin.get(ch);

  proxy.ChangeState(FairMQProxy::STOP);
  proxy.ChangeState(FairMQProxy::END);

  return 0;
}


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


FairMQBuffer buffer;

static void s_signal_handler (int signal)
{
  std::cout << std::endl << "Caught signal " << signal << std::endl;

  buffer.ChangeState(FairMQBuffer::STOP);
  buffer.ChangeState(FairMQBuffer::END);

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
    std::cout << "Usage: buffer \tID numIoTreads\n"
              << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
              << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n" << std::endl;
    return 1;
  }

  s_catch_signals();

  std::stringstream logmsg;
  logmsg << "PID: " << getpid();
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  int i = 1;

  buffer.SetProperty(FairMQBuffer::Id, argv[i]);
  ++i;

  int numIoThreads;
  std::stringstream(argv[i]) >> numIoThreads;
  buffer.SetProperty(FairMQBuffer::NumIoThreads, numIoThreads);
  ++i;
  buffer.SetProperty(FairMQBuffer::NumInputs, 1);
  buffer.SetProperty(FairMQBuffer::NumOutputs, 1);


  buffer.ChangeState(FairMQBuffer::INIT);


  int inputSocketType = ZMQ_SUB;
  if (strcmp(argv[i], "pull") == 0) {
    inputSocketType = ZMQ_PULL;
  }
  buffer.SetProperty(FairMQBuffer::InputSocketType, inputSocketType, 0);
  ++i;
  int inputRcvBufSize;
  std::stringstream(argv[i]) >> inputRcvBufSize;
  buffer.SetProperty(FairMQBuffer::InputRcvBufSize, inputRcvBufSize, 0);
  ++i;
  buffer.SetProperty(FairMQBuffer::InputMethod, argv[i], 0);
  ++i;
  buffer.SetProperty(FairMQBuffer::InputAddress, argv[i], 0);
  ++i;


  int outputSocketType = ZMQ_PUB;
  if (strcmp(argv[i], "push") == 0) {
    outputSocketType = ZMQ_PUSH;
  }
  buffer.SetProperty(FairMQBuffer::OutputSocketType, outputSocketType, 0);
  ++i;
  int outputSndBufSize;
  std::stringstream(argv[i]) >> outputSndBufSize;
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
  std::cin.get(ch);

  buffer.ChangeState(FairMQBuffer::STOP);
  buffer.ChangeState(FairMQBuffer::END);

  return 0;
}


/**
 * runProxy.cxx
 *
 * @since 2013-10-07
 * @author A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQProxy.h"
#include "FairMQTransportFactoryZMQ.h"
// #include "FairMQTransportFactoryNN.h"

using std::cout;
using std::cin;
using std::endl;
using std::stringstream;


FairMQProxy proxy;

static void s_signal_handler (int signal)
{
  cout << endl << "Caught signal " << signal << endl;

  proxy.ChangeState(FairMQProxy::STOP);
  proxy.ChangeState(FairMQProxy::END);

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
  if ( argc != 11 ) {
    cout << "Usage: proxy \tID numIoTreads\n"
              << "\t\tinputSocketType inputRcvBufSize inputMethod inputAddress\n"
              << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n" << endl;
    return 1;
  }

  s_catch_signals();

  stringstream logmsg;
  logmsg << "PID: " << getpid();
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
  // FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
  proxy.SetTransport(transportFactory);

  int i = 1;

  proxy.SetProperty(FairMQProxy::Id, argv[i]);
  ++i;

  int numIoThreads;
  stringstream(argv[i]) >> numIoThreads;
  proxy.SetProperty(FairMQProxy::NumIoThreads, numIoThreads);
  ++i;

  proxy.SetProperty(FairMQProxy::NumInputs, 1);
  proxy.SetProperty(FairMQProxy::NumOutputs, 1);


  proxy.ChangeState(FairMQProxy::INIT);


  proxy.SetProperty(FairMQProxy::InputSocketType, argv[i], 0);
  ++i;
  int inputRcvBufSize;
  stringstream(argv[i]) >> inputRcvBufSize;
  proxy.SetProperty(FairMQProxy::InputRcvBufSize, inputRcvBufSize, 0);
  ++i;
  proxy.SetProperty(FairMQProxy::InputMethod, argv[i], 0);
  ++i;
  proxy.SetProperty(FairMQProxy::InputAddress, argv[i], 0);
  ++i;

  proxy.SetProperty(FairMQProxy::OutputSocketType, argv[i], 0);
  ++i;
  int outputSndBufSize;
  stringstream(argv[i]) >> outputSndBufSize;
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
  cin.get(ch);

  proxy.ChangeState(FairMQProxy::STOP);
  proxy.ChangeState(FairMQProxy::END);

  return 0;
}


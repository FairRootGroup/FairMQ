/**
 * runBenchmarkSampler.cxx
 *
 * @since Apr 23, 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQBenchmarkSampler.h"
#include "FairMQTransportFactoryZMQ.h"


FairMQBenchmarkSampler sampler;

static void s_signal_handler (int signal)
{
  std::cout << std::endl << "Caught signal " << signal << std::endl;

  sampler.ChangeState(FairMQBenchmarkSampler::STOP);
  sampler.ChangeState(FairMQBenchmarkSampler::END);

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
  if ( argc != 9 ) {
    std::cout << "Usage: bsampler ID eventSize eventRate numIoTreads\n"
              << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n"
              << std::endl;
    return 1;
  }

  s_catch_signals();

  std::stringstream logmsg;
  logmsg << "PID: " << getpid();
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
  sampler.SetTransport(transportFactory);

  int i = 1;

  sampler.SetProperty(FairMQBenchmarkSampler::Id, argv[i]);
  ++i;

  int eventSize;
  std::stringstream(argv[i]) >> eventSize;
  sampler.SetProperty(FairMQBenchmarkSampler::EventSize, eventSize);
  ++i;

  int eventRate;
  std::stringstream(argv[i]) >> eventRate;
  sampler.SetProperty(FairMQBenchmarkSampler::EventRate, eventRate);
  ++i;

  int numIoThreads;
  std::stringstream(argv[i]) >> numIoThreads;
  sampler.SetProperty(FairMQBenchmarkSampler::NumIoThreads, numIoThreads);
  ++i;

  sampler.SetProperty(FairMQBenchmarkSampler::NumInputs, 0);
  sampler.SetProperty(FairMQBenchmarkSampler::NumOutputs, 1);


  sampler.ChangeState(FairMQBenchmarkSampler::INIT);


  int outputSocketType = ZMQ_PUB;
  if (strcmp(argv[i], "push") == 0) {
    outputSocketType = ZMQ_PUSH;
  }
  sampler.SetProperty(FairMQBenchmarkSampler::OutputSocketType, outputSocketType, 0);
  ++i;
  int outputSndBufSize;
  std::stringstream(argv[i]) >> outputSndBufSize;
  sampler.SetProperty(FairMQBenchmarkSampler::OutputSndBufSize, outputSndBufSize, 0);
  ++i;
  sampler.SetProperty(FairMQBenchmarkSampler::OutputMethod, argv[i], 0);
  ++i;
  sampler.SetProperty(FairMQBenchmarkSampler::OutputAddress, argv[i], 0);
  ++i;


  sampler.ChangeState(FairMQBenchmarkSampler::SETOUTPUT);
  sampler.ChangeState(FairMQBenchmarkSampler::SETINPUT);
  sampler.ChangeState(FairMQBenchmarkSampler::RUN);



  char ch;
  std::cin.get(ch);

  sampler.ChangeState(FairMQBenchmarkSampler::STOP);
  sampler.ChangeState(FairMQBenchmarkSampler::END);

  return 0;
}


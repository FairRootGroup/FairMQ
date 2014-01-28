/**
 * runBenchmarkSampler.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <csignal>

#include "FairMQLogger.h"
#include "FairMQBenchmarkSampler.h"
#include "FairMQTransportFactoryZMQ.h"

using std::cout;
using std::cin;
using std::endl;
using std::stringstream;


FairMQBenchmarkSampler sampler;

static void s_signal_handler (int signal)
{
  cout << endl << "Caught signal " << signal << endl;

  sampler.ChangeState(FairMQBenchmarkSampler::STOP);
  sampler.ChangeState(FairMQBenchmarkSampler::END);

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
  if ( argc != 9 ) {
    cout << "Usage: bsampler ID eventSize eventRate numIoTreads\n"
              << "\t\toutputSocketType outputSndBufSize outputMethod outputAddress\n"
              << endl;
    return 1;
  }

  s_catch_signals();

  stringstream logmsg;
  logmsg << "PID: " << getpid();
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
  sampler.SetTransport(transportFactory);

  int i = 1;

  sampler.SetProperty(FairMQBenchmarkSampler::Id, argv[i]);
  ++i;

  int eventSize;
  stringstream(argv[i]) >> eventSize;
  sampler.SetProperty(FairMQBenchmarkSampler::EventSize, eventSize);
  ++i;

  int eventRate;
  stringstream(argv[i]) >> eventRate;
  sampler.SetProperty(FairMQBenchmarkSampler::EventRate, eventRate);
  ++i;

  int numIoThreads;
  stringstream(argv[i]) >> numIoThreads;
  sampler.SetProperty(FairMQBenchmarkSampler::NumIoThreads, numIoThreads);
  ++i;

  sampler.SetProperty(FairMQBenchmarkSampler::NumInputs, 0);
  sampler.SetProperty(FairMQBenchmarkSampler::NumOutputs, 1);


  sampler.ChangeState(FairMQBenchmarkSampler::INIT);


  sampler.SetProperty(FairMQBenchmarkSampler::OutputSocketType, argv[i], 0);
  ++i;
  int outputSndBufSize;
  stringstream(argv[i]) >> outputSndBufSize;
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
  cin.get(ch);

  sampler.ChangeState(FairMQBenchmarkSampler::STOP);
  sampler.ChangeState(FairMQBenchmarkSampler::END);

  return 0;
}


/**
 * FairMQDevice.cxx
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>

#include "FairMQSocket.h"
#include "FairMQDevice.h"
#include "FairMQLogger.h"

FairMQDevice::FairMQDevice() :
  fNumIoThreads(1),
  //fPayloadContext(NULL),
  fPayloadInputs(new vector<FairMQSocket*>()),
  fPayloadOutputs(new vector<FairMQSocket*>()),
  fLogIntervalInMs(1000)
{
}

void FairMQDevice::Init()
{
  LOG(INFO) << ">>>>>>> Init <<<<<<<";
  LOG(INFO) << "numIoThreads: " << fNumIoThreads;

  // fPayloadContext = new FairMQContextZMQ(fNumIoThreads);

  // TODO: nafiga?
  fInputAddress = new vector<string>(fNumInputs);
  fInputMethod = new vector<string>();
  fInputSocketType = new vector<string>();
  fInputSndBufSize = new vector<int>();
  fInputRcvBufSize = new vector<int>();

  for (int i = 0; i < fNumInputs; ++i) {
    fInputMethod->push_back("connect"); // default value, can be overwritten in configuration
    fInputSocketType->push_back("sub"); // default value, can be overwritten in configuration
    fInputSndBufSize->push_back(10000); // default value, can be overwritten in configuration
    fInputRcvBufSize->push_back(10000); // default value, can be overwritten in configuration
  }

  fOutputAddress = new vector<string>(fNumOutputs);
  fOutputMethod = new vector<string>();
  fOutputSocketType = new vector<string>();
  fOutputSndBufSize = new vector<int>();
  fOutputRcvBufSize = new vector<int>();

  for (int i = 0; i < fNumOutputs; ++i) {
    fOutputMethod->push_back("bind"); // default value, can be overwritten in configuration
    fOutputSocketType->push_back("pub"); // default value, can be overwritten in configuration
    fOutputSndBufSize->push_back(10000); // default value, can be overwritten in configuration
    fOutputRcvBufSize->push_back(10000); // default value, can be overwritten in configuration
  }
}

void FairMQDevice::InitInput()
{
  LOG(INFO) << ">>>>>>> InitInput <<<<<<<";

  for (int i = 0; i < fNumInputs; ++i) {
    FairMQSocket* socket = fTransportFactory->CreateSocket(fInputSocketType->at(i), i);

    socket->SetOption("snd-hwm", &fInputSndBufSize->at(i), sizeof(fInputSndBufSize->at(i)));
    socket->SetOption("rcv-hwm", &fInputRcvBufSize->at(i), sizeof(fInputRcvBufSize->at(i)));

    fPayloadInputs->push_back(socket);

    try {
      if (fInputMethod->at(i) == "bind") {
        fPayloadInputs->at(i)->Bind(fInputAddress->at(i));
      } else {
        fPayloadInputs->at(i)->Connect(fInputAddress->at(i));
      }
    } catch (std::out_of_range& e) {
    }
  }
}

void FairMQDevice::InitOutput()
{
  LOG(INFO) << ">>>>>>> InitOutput <<<<<<<";

  for (int i = 0; i < fNumOutputs; ++i) {
    FairMQSocket* socket = fTransportFactory->CreateSocket(fOutputSocketType->at(i), i);

    socket->SetOption("snd-hwm", &fOutputSndBufSize->at(i), sizeof(fOutputSndBufSize->at(i)));
    socket->SetOption("rcv-hwm", &fOutputRcvBufSize->at(i), sizeof(fOutputRcvBufSize->at(i)));

    fPayloadOutputs->push_back(socket);

    try {
      if (fOutputMethod->at(i) == "bind") {
        fPayloadOutputs->at(i)->Bind(fOutputAddress->at(i));
      } else {
        fPayloadOutputs->at(i)->Connect(fOutputAddress->at(i));
      }
    } catch (std::out_of_range& e) {
    }
  }
}

void FairMQDevice::Run()
{
}

void FairMQDevice::Pause()
{
}

// Method for setting properties represented as a string.
void FairMQDevice::SetProperty(const int key, const string& value, const int slot/*= 0*/)
{
  switch (key) {
  case Id:
    fId = value;
    break;
  case InputAddress:
    fInputAddress->erase(fInputAddress->begin() + slot);
    fInputAddress->insert(fInputAddress->begin() + slot, value);
    break;
  case OutputAddress:
    fOutputAddress->erase(fOutputAddress->begin() + slot);
    fOutputAddress->insert(fOutputAddress->begin() + slot, value);
    break;
  case InputMethod:
    fInputMethod->erase(fInputMethod->begin() + slot);
    fInputMethod->insert(fInputMethod->begin() + slot, value);
    break;
  case OutputMethod:
    fOutputMethod->erase(fOutputMethod->begin() + slot);
    fOutputMethod->insert(fOutputMethod->begin() + slot, value);
    break;
  case InputSocketType:
    fInputSocketType->erase(fInputSocketType->begin() + slot);
    fInputSocketType->insert(fInputSocketType->begin() + slot, value);
    break;
  case OutputSocketType:
    fOutputSocketType->erase(fOutputSocketType->begin() + slot);
    fOutputSocketType->insert(fOutputSocketType->begin() + slot, value);
    break;
  default:
    FairMQConfigurable::SetProperty(key, value, slot);
    break;
  }
}

// Method for setting properties represented as an integer.
void FairMQDevice::SetProperty(const int key, const int value, const int slot/*= 0*/)
{
  switch (key) {
  case NumIoThreads:
    fNumIoThreads = value;
    break;
  case NumInputs:
    fNumInputs = value;
    break;
  case NumOutputs:
    fNumOutputs = value;
    break;
  case LogIntervalInMs:
    fLogIntervalInMs = value;
    break;
  case InputSndBufSize:
    fInputSndBufSize->erase(fInputSndBufSize->begin() + slot);
    fInputSndBufSize->insert(fInputSndBufSize->begin() + slot, value);
    break;
  case InputRcvBufSize:
    fInputRcvBufSize->erase(fInputRcvBufSize->begin() + slot);
    fInputRcvBufSize->insert(fInputRcvBufSize->begin() + slot, value);
    break;
  case OutputSndBufSize:
    fOutputSndBufSize->erase(fOutputSndBufSize->begin() + slot);
    fOutputSndBufSize->insert(fOutputSndBufSize->begin() + slot, value);
    break;
  case OutputRcvBufSize:
    fOutputRcvBufSize->erase(fOutputRcvBufSize->begin() + slot);
    fOutputRcvBufSize->insert(fOutputRcvBufSize->begin() + slot, value);
    break;
  default:
    FairMQConfigurable::SetProperty(key, value, slot);
    break;
  }
}

// Method for getting properties represented as an string.
string FairMQDevice::GetProperty(const int key, const string& default_/*= ""*/, const int slot/*= 0*/)
{
  switch (key) {
  case Id:
    return fId;
  case InputAddress:
    return fInputAddress->at(slot);
  case OutputAddress:
    return fOutputAddress->at(slot);
  case InputMethod:
    return fInputMethod->at(slot);
  case OutputMethod:
    return fOutputMethod->at(slot);
  case InputSocketType:
    return fInputSocketType->at(slot);
  case OutputSocketType:
    return fOutputSocketType->at(slot);
  default:
    return FairMQConfigurable::GetProperty(key, default_, slot);
  }
}

// Method for getting properties represented as an integer.
int FairMQDevice::GetProperty(const int key, const int default_/*= 0*/, const int slot/*= 0*/)
{
  switch (key) {
  case NumIoThreads:
    return fNumIoThreads;
  case LogIntervalInMs:
    return fLogIntervalInMs;
  case InputSndBufSize:
    return fInputSndBufSize->at(slot);
  case InputRcvBufSize:
    return fInputRcvBufSize->at(slot);
  case OutputSndBufSize:
    return fOutputSndBufSize->at(slot);
  case OutputRcvBufSize:
    return fOutputRcvBufSize->at(slot);
  default:
    return FairMQConfigurable::GetProperty(key, default_, slot);
  }
}

void FairMQDevice::SetTransport(FairMQTransportFactory* factory)
{
  fTransportFactory = factory;
}

void FairMQDevice::LogSocketRates()
{
  timestamp_t t0;
  timestamp_t t1;

  timestamp_t timeSinceLastLog_ms;

  unsigned long* bytesInput = new unsigned long[fNumInputs];
  unsigned long* messagesInput = new unsigned long[fNumInputs];
  unsigned long* bytesOutput = new unsigned long[fNumOutputs];
  unsigned long* messagesOutput = new unsigned long[fNumOutputs];

  unsigned long* bytesInputNew = new unsigned long[fNumInputs];
  unsigned long* messagesInputNew = new unsigned long[fNumInputs];
  unsigned long* bytesOutputNew = new unsigned long[fNumOutputs];
  unsigned long* messagesOutputNew = new unsigned long[fNumOutputs];

  double* megabytesPerSecondInput = new double[fNumInputs];
  double* messagesPerSecondInput = new double[fNumInputs];
  double* megabytesPerSecondOutput = new double[fNumOutputs];
  double* messagesPerSecondOutput = new double[fNumOutputs];

  // Temp stuff for process termination
  // bool receivedSomething = false;
  // bool sentSomething = false;
  // int didNotReceiveFor = 0;
  // int didNotSendFor = 0;
  // End of temp stuff

  int i = 0;
  for ( vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++ ) {
    bytesInput[i] = (*itr)->GetBytesRx();
    messagesInput[i] = (*itr)->GetMessagesRx();
    ++i;
  }

  i = 0;
  for ( vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++ ) {
    bytesOutput[i] = (*itr)->GetBytesTx();
    messagesOutput[i] = (*itr)->GetMessagesTx();
    ++i;
  }

  t0 = get_timestamp();

  while ( true ) {
    try {
      t1 = get_timestamp();

      timeSinceLastLog_ms = (t1 - t0) / 1000.0L;

      i = 0;

      for ( vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++ ) {
        bytesInputNew[i] = (*itr)->GetBytesRx();
        megabytesPerSecondInput[i] = ((double) (bytesInputNew[i] - bytesInput[i]) / (1024. * 1024.)) / (double) timeSinceLastLog_ms * 1000.;
        bytesInput[i] = bytesInputNew[i];
        messagesInputNew[i] = (*itr)->GetMessagesRx();
        messagesPerSecondInput[i] = (double) (messagesInputNew[i] - messagesInput[i]) / (double) timeSinceLastLog_ms * 1000.;
        messagesInput[i] = messagesInputNew[i];

        LOG(DEBUG) << "#" << fId << "." << (*itr)->GetId() << ": " << messagesPerSecondInput[i] << " msg/s, " << megabytesPerSecondInput[i] << " MB/s";

        // Temp stuff for process termination
        // if ( !receivedSomething && messagesPerSecondInput[i] > 0 ) {
     //      receivedSomething = true;
     //    }
     //    if ( receivedSomething && messagesPerSecondInput[i] == 0 ) {
     //      cout << "Did not receive anything on socket " << i << " for " << didNotReceiveFor++ << " seconds." << endl;
     //    } else {
     //      didNotReceiveFor = 0;
     //    }
        // End of temp stuff

        ++i;
      }

      i = 0;

      for ( vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++ )
      {
        bytesOutputNew[i] = (*itr)->GetBytesTx();
        megabytesPerSecondOutput[i] = ((double) (bytesOutputNew[i] - bytesOutput[i]) / (1024. * 1024.)) / (double) timeSinceLastLog_ms * 1000.;
        bytesOutput[i] = bytesOutputNew[i];
        messagesOutputNew[i] = (*itr)->GetMessagesTx();
        messagesPerSecondOutput[i] = (double) (messagesOutputNew[i] - messagesOutput[i]) / (double) timeSinceLastLog_ms * 1000.;
        messagesOutput[i] = messagesOutputNew[i];

        LOG(DEBUG) << "#" << fId << "." << (*itr)->GetId() << ": " << messagesPerSecondOutput[i] << " msg/s, " << megabytesPerSecondOutput[i] << " MB/s";

        // Temp stuff for process termination
        // if ( !sentSomething && messagesPerSecondOutput[i] > 0 ) {
     //      sentSomething = true;
     //    }
     //    if ( sentSomething && messagesPerSecondOutput[i] == 0 ) {
     //      cout << "Did not send anything on socket " << i << " for " << didNotSendFor++ << " seconds." << endl;
     //    } else {
     //      didNotSendFor = 0;
     //    }
        // End of temp stuff

        ++i;
      }

      // Temp stuff for process termination
      // if (receivedSomething && didNotReceiveFor > 5) {
     //    cout << "stopping because nothing was received for 5 seconds." << endl;
     //    ChangeState(STOP);
     //  }
     //  if (sentSomething && didNotSendFor > 5) {
     //    cout << "stopping because nothing was sent for 5 seconds." << endl;
     //    ChangeState(STOP);
     //  }
      // End of temp stuff

      t0 = t1;
      boost::this_thread::sleep(boost::posix_time::milliseconds(fLogIntervalInMs));
    } catch (boost::thread_interrupted&) {
      cout << "rateLogger interrupted" << endl;
      break;
    }
  }

  delete[] bytesInput;
  delete[] messagesInput;
  delete[] bytesOutput;
  delete[] messagesOutput;

  delete[] bytesInputNew;
  delete[] messagesInputNew;
  delete[] bytesOutputNew;
  delete[] messagesOutputNew;

  delete[] megabytesPerSecondInput;
  delete[] messagesPerSecondInput;
  delete[] megabytesPerSecondOutput;
  delete[] messagesPerSecondOutput;

  LOG(INFO) << ">>>>>>> stopping rateLogger <<<<<<<";
}

void FairMQDevice::ListenToCommands()
{
}

void FairMQDevice::Shutdown()
{
  LOG(INFO) << ">>>>>>> closing inputs <<<<<<<";
  for( vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++ ) {
    (*itr)->Close();
  }

  LOG(INFO) << ">>>>>>> closing outputs <<<<<<<";
  for( vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++ ) {
    (*itr)->Close();
  }

  // LOG(INFO) << ">>>>>>> closing context <<<<<<<";
  // fPayloadContext->Close();
}

FairMQDevice::~FairMQDevice()
{
  for( vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++ ) {
    delete (*itr);
  }

  for( vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++ ) {
    delete (*itr);
  }

  delete fInputAddress;
  delete fOutputAddress;
  delete fPayloadInputs;
  delete fPayloadOutputs;
}


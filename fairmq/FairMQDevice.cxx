/**
 * FairMQDevice.cxx
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>

#include <boost/thread.hpp>

#include "FairMQSocket.h"
#include "FairMQDevice.h"
#include "FairMQLogger.h"

FairMQDevice::FairMQDevice() :
  fId(""),
  fNumIoThreads(1),
  fPayloadContext(NULL),
  fPayloadInputs(new std::vector<FairMQSocket*>()),
  fPayloadOutputs(new std::vector<FairMQSocket*>()),
  fLogIntervalInMs(1000)
{
}

void FairMQDevice::Init()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Init <<<<<<<");
  std::stringstream logmsg;
  logmsg << "numIoThreads: " << fNumIoThreads;
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, logmsg.str());

  fPayloadContext = new FairMQContext(fNumIoThreads);

  fInputAddress = new std::vector<TString>(fNumInputs);
  fInputMethod = new std::vector<TString>();
  fInputSocketType = new std::vector<Int_t>();
  fInputSndBufSize = new std::vector<Int_t>();
  fInputRcvBufSize = new std::vector<Int_t>();

  for (Int_t i = 0; i < fNumInputs; ++i) {
    fInputMethod->push_back("connect"); // default value, can be overwritten in configuration
    fInputSocketType->push_back(ZMQ_SUB); // default value, can be overwritten in configuration
    fInputSndBufSize->push_back(10000); // default value, can be overwritten in configuration
    fInputRcvBufSize->push_back(10000); // default value, can be overwritten in configuration
  }

  fOutputAddress = new std::vector<TString>(fNumOutputs);
  fOutputMethod = new std::vector<TString>();
  fOutputSocketType = new std::vector<Int_t>();
  fOutputSndBufSize = new std::vector<Int_t>();
  fOutputRcvBufSize = new std::vector<Int_t>();

  for (Int_t i = 0; i < fNumOutputs; ++i) {
    fOutputMethod->push_back("bind"); // default value, can be overwritten in configuration
    fOutputSocketType->push_back(ZMQ_PUB); // default value, can be overwritten in configuration
    fOutputSndBufSize->push_back(10000); // default value, can be overwritten in configuration
    fOutputRcvBufSize->push_back(10000); // default value, can be overwritten in configuration
  }
}

void FairMQDevice::InitInput()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> InitInput <<<<<<<");

  for (Int_t i = 0; i < fNumInputs; ++i) {
    FairMQSocket* socket = new FairMQSocket(fPayloadContext, fInputSocketType->at(i), i);

    socket->SetOption(ZMQ_SNDHWM, &fInputSndBufSize->at(i), sizeof(fInputSndBufSize->at(i)));
    socket->SetOption(ZMQ_RCVHWM, &fInputRcvBufSize->at(i), sizeof(fInputRcvBufSize->at(i)));

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
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> InitOutput <<<<<<<");

  for (Int_t i = 0; i < fNumOutputs; ++i) {
    FairMQSocket* socket = new FairMQSocket(fPayloadContext, fOutputSocketType->at(i), i);
    socket->SetOption(ZMQ_SNDHWM, &fOutputSndBufSize->at(i), sizeof(fOutputSndBufSize->at(i)));
    socket->SetOption(ZMQ_RCVHWM, &fOutputRcvBufSize->at(i), sizeof(fOutputRcvBufSize->at(i)));
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
void FairMQDevice::SetProperty(const Int_t& key, const TString& value, const Int_t& slot/*= 0*/)
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
  default:
    FairMQConfigurable::SetProperty(key, value, slot);
    break;
  }
}

// Method for setting properties represented as an integer.
void FairMQDevice::SetProperty(const Int_t& key, const Int_t& value, const Int_t& slot/*= 0*/)
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
  case InputSocketType:
    fInputSocketType->erase(fInputSocketType->begin() + slot);
    fInputSocketType->insert(fInputSocketType->begin() + slot, value);
    break;
  case InputSndBufSize:
    fInputSndBufSize->erase(fInputSndBufSize->begin() + slot);
    fInputSndBufSize->insert(fInputSndBufSize->begin() + slot, value);
    break;
  case InputRcvBufSize:
    fInputRcvBufSize->erase(fInputRcvBufSize->begin() + slot);
    fInputRcvBufSize->insert(fInputRcvBufSize->begin() + slot, value);
    break;
  case OutputSocketType:
    fOutputSocketType->erase(fOutputSocketType->begin() + slot);
    fOutputSocketType->insert(fOutputSocketType->begin() + slot, value);
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
TString FairMQDevice::GetProperty(const Int_t& key, const TString& default_/*= ""*/, const Int_t& slot/*= 0*/)
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
  default:
    return FairMQConfigurable::GetProperty(key, default_, slot);
  }
}

// Method for getting properties represented as an integer.
Int_t FairMQDevice::GetProperty(const Int_t& key, const Int_t& default_/*= 0*/, const Int_t& slot/*= 0*/)
{
  switch (key) {
  case NumIoThreads:
    return fNumIoThreads;
  case LogIntervalInMs:
    return fLogIntervalInMs;
  case InputSocketType:
    return fInputSocketType->at(slot);
  case InputSndBufSize:
    return fInputSndBufSize->at(slot);
  case InputRcvBufSize:
    return fInputRcvBufSize->at(slot);
  case OutputSocketType:
    return fOutputSocketType->at(slot);
  case OutputSndBufSize:
    return fOutputSndBufSize->at(slot);
  case OutputRcvBufSize:
    return fOutputRcvBufSize->at(slot);
  default:
    return FairMQConfigurable::GetProperty(key, default_, slot);
  }
}

void FairMQDevice::LogSocketRates()
{
  timestamp_t t0;
  timestamp_t t1;

  timestamp_t timeSinceLastLog_ms;

  ULong_t* bytesInput = new ULong_t[fNumInputs];
  ULong_t* messagesInput = new ULong_t[fNumInputs];
  ULong_t* bytesOutput = new ULong_t[fNumOutputs];
  ULong_t* messagesOutput = new ULong_t[fNumOutputs];

  ULong_t* bytesInputNew = new ULong_t[fNumInputs];
  ULong_t* messagesInputNew = new ULong_t[fNumInputs];
  ULong_t* bytesOutputNew = new ULong_t[fNumOutputs];
  ULong_t* messagesOutputNew = new ULong_t[fNumOutputs];

  Double_t* megabytesPerSecondInput = new Double_t[fNumInputs];
  Double_t* messagesPerSecondInput = new Double_t[fNumInputs];
  Double_t* megabytesPerSecondOutput = new Double_t[fNumOutputs];
  Double_t* messagesPerSecondOutput = new Double_t[fNumOutputs];

  // Temp stuff for process termination
  bool receivedSomething = false;
  bool sentSomething = false;
  int didNotReceiveFor = 0;
  int didNotSendFor = 0;
  // End of temp stuff

  Int_t i = 0;
  for( std::vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++ ) {
    bytesInput[i] = (*itr)->GetBytesRx();
    messagesInput[i] = (*itr)->GetMessagesRx();
    ++i;
  }

  i = 0;
  for( std::vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++ ) {
    bytesOutput[i] = (*itr)->GetBytesTx();
    messagesOutput[i] = (*itr)->GetMessagesTx();
    ++i;
  }

  t0 = get_timestamp();

  while ( true ) {
    try {
      boost::this_thread::sleep(boost::posix_time::milliseconds(fLogIntervalInMs));

      t1 = get_timestamp();

      timeSinceLastLog_ms = (t1 - t0) / 1000.0L;

      i = 0;

      for( std::vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++ ) {
        bytesInputNew[i] = (*itr)->GetBytesRx();
        megabytesPerSecondInput[i] = ((Double_t) (bytesInputNew[i] - bytesInput[i]) / (1024. * 1024.)) / (Double_t) timeSinceLastLog_ms * 1000.;
        bytesInput[i] = bytesInputNew[i];
        messagesInputNew[i] = (*itr)->GetMessagesRx();
        messagesPerSecondInput[i] = (Double_t) (messagesInputNew[i] - messagesInput[i]) / (Double_t) timeSinceLastLog_ms * 1000.;
        messagesInput[i] = messagesInputNew[i];

        std::stringstream logmsg;
        logmsg << "#" << fId << "." << (*itr)->GetId() << ": " << messagesPerSecondInput[i] << " msg/s, " << megabytesPerSecondInput[i] << " MB/s";
        FairMQLogger::GetInstance()->Log(FairMQLogger::DEBUG, logmsg.str());

        // Temp stuff for process termination
        if ( !receivedSomething && messagesPerSecondInput[i] > 0 ) {
          receivedSomething = true;
        }
        if ( receivedSomething && messagesPerSecondInput[i] == 0 ) {
          std::cout << "Did not receive anything on socket " << i << " for " << didNotReceiveFor++ << " seconds." << std::endl;
        } else {
          didNotReceiveFor = 0;
        }
        // End of temp stuff

        ++i;
      }

      i = 0;

      for( std::vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++ ) {
        bytesOutputNew[i] = (*itr)->GetBytesTx();
        megabytesPerSecondOutput[i] = ((Double_t) (bytesOutputNew[i] - bytesOutput[i]) / (1024. * 1024.)) / (Double_t) timeSinceLastLog_ms * 1000.;
        bytesOutput[i] = bytesOutputNew[i];
        messagesOutputNew[i] = (*itr)->GetMessagesTx();
        messagesPerSecondOutput[i] = (Double_t) (messagesOutputNew[i] - messagesOutput[i]) / (Double_t) timeSinceLastLog_ms * 1000.;
        messagesOutput[i] = messagesOutputNew[i];

        std::stringstream logmsg;
        logmsg << "#" << fId << "." << (*itr)->GetId() << ": " << messagesPerSecondOutput[i] << " msg/s, " << megabytesPerSecondOutput[i] << " MB/s";
        FairMQLogger::GetInstance()->Log(FairMQLogger::DEBUG, logmsg.str());

        // Temp stuff for process termination
        if ( !sentSomething && messagesPerSecondOutput[i] > 0 ) {
          sentSomething = true;
        }
        if ( sentSomething && messagesPerSecondOutput[i] == 0 ) {
          std::cout << "Did not send anything on socket " << i << " for " << didNotSendFor++ << " seconds." << std::endl;
        } else {
          didNotSendFor = 0;
        }
        // End of temp stuff

        ++i;
      }

      // Temp stuff for process termination
      if (receivedSomething && didNotReceiveFor > 5) {
        std::cout << "stopping because nothing was received for 5 seconds." << std::endl;
        ChangeState(STOP);
      }
      if (sentSomething && didNotSendFor > 5) {
        std::cout << "stopping because nothing was sent for 5 seconds." << std::endl;
        ChangeState(STOP);
      }
      // End of temp stuff

      t0 = t1;
    } catch (boost::thread_interrupted&) {
      std::cout << "rateLogger interrupted" << std::endl;
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

  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> stopping rateLogger <<<<<<<");
}

void FairMQDevice::ListenToCommands()
{
}

void FairMQDevice::Shutdown()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> closing inputs <<<<<<<");
  for( std::vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++ ) {
    (*itr)->Close();
  }

  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> closing outputs <<<<<<<");
  for( std::vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++ ) {
    (*itr)->Close();
  }

  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> closing context <<<<<<<");
  fPayloadContext->Close();
}

FairMQDevice::~FairMQDevice()
{
  for( std::vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++ ) {
    delete (*itr);
  }

  for( std::vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++ ) {
    delete (*itr);
  }

  delete fInputAddress;
  delete fOutputAddress;
  delete fPayloadInputs;
  delete fPayloadOutputs;
}


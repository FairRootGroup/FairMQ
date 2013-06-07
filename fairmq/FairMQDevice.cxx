/**
 * FairMQDevice.cxx
 *
 *  @since Oct 25, 2012
 *  @authors: D. Klein
 */

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

  fPayloadContext = new FairMQContext(fId, FairMQContext::PAYLOAD, fNumIoThreads);

  fBindAddress = new std::vector<TString>(fNumOutputs);
  fBindSocketType = new std::vector<Int_t>();
  fBindSndBufferSize = new std::vector<Int_t>();
  fBindRcvBufferSize = new std::vector<Int_t>();

  for (Int_t i = 0; i < fNumOutputs; ++i) {
    fBindSocketType->push_back(ZMQ_PUB);
    fBindSndBufferSize->push_back(10000);
    fBindRcvBufferSize->push_back(10000);
  }

  fConnectAddress = new std::vector<TString>(fNumInputs);
  fConnectSocketType = new std::vector<Int_t>();
  fConnectSndBufferSize = new std::vector<Int_t>();
  fConnectRcvBufferSize = new std::vector<Int_t>();

  for (Int_t i = 0; i < fNumInputs; ++i) {
    fConnectSocketType->push_back(ZMQ_SUB);
    fConnectSndBufferSize->push_back(10000);
    fConnectRcvBufferSize->push_back(10000);
  }
}

void FairMQDevice::Bind()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Bind <<<<<<<");

  for (Int_t i = 0; i < fNumOutputs; ++i) {
    FairMQSocket* socket = new FairMQSocket(fPayloadContext, fBindSocketType->at(i), i);
    socket->GetSocket()->setsockopt(ZMQ_SNDHWM, &fBindSndBufferSize->at(i), sizeof(fBindSndBufferSize->at(i)));
    socket->GetSocket()->setsockopt(ZMQ_RCVHWM, &fBindRcvBufferSize->at(i), sizeof(fBindRcvBufferSize->at(i)));
    fPayloadOutputs->push_back(socket);
  }

  Int_t i = 0;

  for( std::vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++ ) {
    try {
      (*itr)->Bind(fBindAddress->at(i));
    } catch (std::out_of_range& e) {
    }

    ++i;
  }

}

void FairMQDevice::Connect()
{
  FairMQLogger::GetInstance()->Log(FairMQLogger::INFO, ">>>>>>> Connect <<<<<<<");

  for (Int_t i = 0; i < fNumInputs; ++i) {
    FairMQSocket* socket = new FairMQSocket(fPayloadContext, fConnectSocketType->at(i), i);
    socket->GetSocket()->setsockopt(ZMQ_SNDHWM, &fConnectSndBufferSize->at(i), sizeof(fConnectSndBufferSize->at(i)));
    socket->GetSocket()->setsockopt(ZMQ_RCVHWM, &fConnectRcvBufferSize->at(i), sizeof(fConnectRcvBufferSize->at(i)));
    fPayloadInputs->push_back(socket);
  }

  Int_t i = 0;

  for( std::vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++ ) {
    try {
      (*itr)->Connect(fConnectAddress->at(i));
    } catch (std::out_of_range& e) {
    }

    ++i;
  }

}

void FairMQDevice::Run()
{
}

void FairMQDevice::Pause()
{
}

void FairMQDevice::Shutdown()
{
  for( std::vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++ ) {
    (*itr)->Close();
  }

  for( std::vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++ ) {
    (*itr)->Close();
  }

  fPayloadContext->Close();
}

void* FairMQDevice::LogSocketRates()
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

  while (true) {
    usleep(fLogIntervalInMs * 1000);

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
      logmsg << "#" << (*itr)->GetId() << ": " << messagesPerSecondInput[i] << " msg/s, " << megabytesPerSecondInput[i] << " MB/s";
      FairMQLogger::GetInstance()->Log(FairMQLogger::DEBUG, logmsg.str());

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
      logmsg << "#" << (*itr)->GetId() << ": " << messagesPerSecondOutput[i] << " msg/s, " << megabytesPerSecondOutput[i] << " MB/s";
      FairMQLogger::GetInstance()->Log(FairMQLogger::DEBUG, logmsg.str());

      ++i;
    }

    t0 = t1;
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
}

void FairMQDevice::SetProperty(Int_t key, TString value, Int_t slot/*= 0*/)
{
  switch (key) {
  case Id:
    fId = value;
    break;
  case BindAddress:
    fBindAddress->erase(fBindAddress->begin() + slot);
    fBindAddress->insert(fBindAddress->begin() + slot, value);
    break;
  case ConnectAddress:
    fConnectAddress->erase(fConnectAddress->begin() + slot);
    fConnectAddress->insert(fConnectAddress->begin() + slot, value);
    break;
  default:
    FairMQConfigurable::SetProperty(key, value, slot);
    break;
  }
}

TString FairMQDevice::GetProperty(Int_t key, TString default_/*= ""*/, Int_t slot/*= 0*/)
{
  switch (key) {
  case Id:
    return fId;
  case BindAddress:
    return fBindAddress->at(slot);
  case ConnectAddress:
    return fConnectAddress->at(slot);
  default:
    return FairMQConfigurable::GetProperty(key, default_, slot);
  }
}

void FairMQDevice::SetProperty(Int_t key, Int_t value, Int_t slot/*= 0*/)
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
  case BindSocketType:
    fBindSocketType->erase(fBindSocketType->begin() + slot);
    fBindSocketType->insert(fBindSocketType->begin() + slot, value);
    break;
  case BindSndBufferSize:
    fBindSndBufferSize->erase(fBindSndBufferSize->begin() + slot);
    fBindSndBufferSize->insert(fBindSndBufferSize->begin() + slot, value);
    break;
  case BindRcvBufferSize:
    fBindRcvBufferSize->erase(fBindRcvBufferSize->begin() + slot);
    fBindRcvBufferSize->insert(fBindRcvBufferSize->begin() + slot, value);
    break;
  case ConnectSocketType:
    fConnectSocketType->erase(fConnectSocketType->begin() + slot);
    fConnectSocketType->insert(fConnectSocketType->begin() + slot, value);
    break;
  case ConnectSndBufferSize:
    fConnectSndBufferSize->erase(fConnectSndBufferSize->begin() + slot);
    fConnectSndBufferSize->insert(fConnectSndBufferSize->begin() + slot, value);
    break;
  case ConnectRcvBufferSize:
    fConnectRcvBufferSize->erase(fConnectRcvBufferSize->begin() + slot);
    fConnectRcvBufferSize->insert(fConnectRcvBufferSize->begin() + slot, value);
    break;
  default:
    FairMQConfigurable::SetProperty(key, value, slot);
    break;
  }
}

Int_t FairMQDevice::GetProperty(Int_t key, Int_t default_/*= 0*/, Int_t slot/*= 0*/)
{
  switch (key) {
  case NumIoThreads:
    return fNumIoThreads;
  case LogIntervalInMs:
    return fLogIntervalInMs;
  case BindSocketType:
    return fBindSocketType->at(slot);
  case ConnectSocketType:
    return fConnectSocketType->at(slot);
  case ConnectSndBufferSize:
    return fConnectSndBufferSize->at(slot);
  case ConnectRcvBufferSize:
    return fConnectRcvBufferSize->at(slot);
  case BindSndBufferSize:
    return fBindSndBufferSize->at(slot);
  case BindRcvBufferSize:
    return fBindRcvBufferSize->at(slot);
  default:
    return FairMQConfigurable::GetProperty(key, default_, slot);
  }
}

FairMQDevice::~FairMQDevice()
{
  for( std::vector<FairMQSocket*>::iterator itr = fPayloadInputs->begin(); itr != fPayloadInputs->end(); itr++ ) {
    delete (*itr);
  }

  for( std::vector<FairMQSocket*>::iterator itr = fPayloadOutputs->begin(); itr != fPayloadOutputs->end(); itr++ ) {
    delete (*itr);
  }

  delete fBindAddress;
  delete fConnectAddress;
  delete fPayloadInputs;
  delete fPayloadOutputs;
}


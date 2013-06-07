/**
 * FairMQDevice.h
 *
 *  @since Oct 25, 2012
 *  @authors: D. Klein
 */

#ifndef FAIRMQDEVICE_H_
#define FAIRMQDEVICE_H_

#include "FairMQConfigurable.h"
#include "FairMQStateMachine.h"
#include <vector>
#include "FairMQContext.h"
#include "FairMQSocket.h"
#include <stdexcept>
//#include "FairMQLogger.h"
#include "Rtypes.h"
#include "TString.h"


class FairMQDevice : /*public FairMQStateMachine,*/ public FairMQConfigurable
{
  protected:
    TString fId;
    Int_t fNumIoThreads;
    FairMQContext* fPayloadContext;
    std::vector<TString> *fBindAddress;
    std::vector<Int_t> *fBindSocketType;
    std::vector<Int_t> *fBindSndBufferSize;
    std::vector<Int_t> *fBindRcvBufferSize;
    std::vector<TString> *fConnectAddress;
    std::vector<Int_t> *fConnectSocketType;
    std::vector<Int_t> *fConnectSndBufferSize;
    std::vector<Int_t> *fConnectRcvBufferSize;
    std::vector<FairMQSocket*> *fPayloadInputs;
    std::vector<FairMQSocket*> *fPayloadOutputs;
    Int_t fLogIntervalInMs;
    Int_t fNumInputs;
    Int_t fNumOutputs;
  public:
    enum {
      Id = FairMQConfigurable::Last,
      NumIoThreads,
      NumInputs,
      NumOutputs,
      BindAddress,
      BindSocketType,
      BindSndBufferSize,
      BindRcvBufferSize,
      ConnectAddress,
      ConnectSocketType,
      ConnectSndBufferSize,
      ConnectRcvBufferSize,
      LogIntervalInMs,
      Last
    };
    FairMQDevice();
    virtual void Init();
    virtual void Bind();
    virtual void Connect();
    virtual void Run();
    virtual void Pause();
    virtual void Shutdown();
    virtual void* LogSocketRates();
    static void* callLogSocketRates(void* arg) { return ((FairMQDevice*)arg)->LogSocketRates(); }
    virtual void SetProperty(Int_t key, TString value, Int_t slot = 0);
    virtual TString GetProperty(Int_t key, TString default_ = "", Int_t slot = 0);
    virtual void SetProperty(Int_t key, Int_t value, Int_t slot = 0);
    virtual Int_t GetProperty(Int_t key, Int_t default_ = 0, Int_t slot = 0);
    virtual ~FairMQDevice();
};

#endif /* FAIRMQDEVICE_H_ */

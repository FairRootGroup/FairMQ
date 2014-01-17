/**
 * FairMQDevice.h
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQDEVICE_H_
#define FAIRMQDEVICE_H_

#include "FairMQConfigurable.h"
#include "FairMQStateMachine.h"
#include <vector>
#include "FairMQContext.h"
#include "FairMQSocket.h"
#include "Rtypes.h"
#include "TString.h"


class FairMQDevice : public FairMQStateMachine, public FairMQConfigurable
{
  public:
    enum {
      Id = FairMQConfigurable::Last,
      NumIoThreads,
      NumInputs,
      NumOutputs,
      InputAddress,
      InputMethod,
      InputSocketType,
      InputSndBufSize,
      InputRcvBufSize,
      OutputAddress,
      OutputMethod,
      OutputSocketType,
      OutputSndBufSize,
      OutputRcvBufSize,
      LogIntervalInMs,
      Last
    };

    FairMQDevice();

    virtual void LogSocketRates();
    virtual void ListenToCommands();

    virtual void SetProperty(const Int_t& key, const TString& value, const Int_t& slot = 0);
    virtual TString GetProperty(const Int_t& key, const TString& default_ = "", const Int_t& slot = 0);
    virtual void SetProperty(const Int_t& key, const Int_t& value, const Int_t& slot = 0);
    virtual Int_t GetProperty(const Int_t& key, const Int_t& default_ = 0, const Int_t& slot = 0);

    virtual ~FairMQDevice();

  protected:
    TString fId;
    Int_t fNumIoThreads;
    FairMQContext* fPayloadContext;

    Int_t fNumInputs;
    Int_t fNumOutputs;

    std::vector<TString> *fInputAddress;
    std::vector<TString> *fInputMethod;
    std::vector<Int_t> *fInputSocketType;
    std::vector<Int_t> *fInputSndBufSize;
    std::vector<Int_t> *fInputRcvBufSize;

    std::vector<TString> *fOutputAddress;
    std::vector<TString> *fOutputMethod;
    std::vector<Int_t> *fOutputSocketType;
    std::vector<Int_t> *fOutputSndBufSize;
    std::vector<Int_t> *fOutputRcvBufSize;

    std::vector<FairMQSocket*> *fPayloadInputs;
    std::vector<FairMQSocket*> *fPayloadOutputs;

    Int_t fLogIntervalInMs;

    virtual void Init();
    virtual void Run();
    virtual void Pause();
    virtual void Shutdown();
    virtual void InitOutput();
    virtual void InitInput();
};

#endif /* FAIRMQDEVICE_H_ */

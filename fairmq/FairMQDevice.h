/**
 * FairMQDevice.h
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQDEVICE_H_
#define FAIRMQDEVICE_H_

#include <vector>
#include <string>

#include "FairMQConfigurable.h"
#include "FairMQStateMachine.h"
#include "FairMQTransportFactory.h"
#include "FairMQContext.h"
#include "FairMQSocket.h"


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

    virtual void SetProperty(const int& key, const std::string& value, const int& slot = 0);
    virtual std::string GetProperty(const int& key, const std::string& default_ = "", const int& slot = 0);
    virtual void SetProperty(const int& key, const int& value, const int& slot = 0);
    virtual int GetProperty(const int& key, const int& default_ = 0, const int& slot = 0);

    virtual void SetTransport(FairMQTransportFactory* factory);

    virtual ~FairMQDevice();

  protected:
    std::string fId;
    int fNumIoThreads;
    FairMQContext* fPayloadContext;
    FairMQTransportFactory* fTransportFactory;

    int fNumInputs;
    int fNumOutputs;

    std::vector<std::string> *fInputAddress;
    std::vector<std::string> *fInputMethod;
    std::vector<int> *fInputSocketType;
    std::vector<int> *fInputSndBufSize;
    std::vector<int> *fInputRcvBufSize;

    std::vector<std::string> *fOutputAddress;
    std::vector<std::string> *fOutputMethod;
    std::vector<int> *fOutputSocketType;
    std::vector<int> *fOutputSndBufSize;
    std::vector<int> *fOutputRcvBufSize;

    std::vector<FairMQSocket*> *fPayloadInputs;
    std::vector<FairMQSocket*> *fPayloadOutputs;

    int fLogIntervalInMs;

    virtual void Init();
    virtual void Run();
    virtual void Pause();
    virtual void Shutdown();
    virtual void InitOutput();
    virtual void InitInput();
};

#endif /* FAIRMQDEVICE_H_ */

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
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
#include <iostream>

#include "FairMQConfigurable.h"
#include "FairMQStateMachine.h"
#include "FairMQTransportFactory.h"
#include "FairMQSocket.h"

class FairMQDevice : public FairMQStateMachine, public FairMQConfigurable
{
  public:
    enum
    {
        Id = FairMQConfigurable::Last,
        NumIoThreads,
        NumInputs,
        NumOutputs,
        PortRangeMin,
        PortRangeMax,
        InputAddress,
        InputMethod,
        InputSocketType,
        InputSndBufSize,
        InputRcvBufSize,
        InputRateLogging,
        OutputAddress,
        OutputMethod,
        OutputSocketType,
        OutputSndBufSize,
        OutputRcvBufSize,
        OutputRateLogging,
        LogInputRate, // keep this for backwards compatibility for a while
        LogOutputRate, // keep this for backwards compatibility for a while
        LogIntervalInMs,
        Last
    };

    FairMQDevice();

    virtual void LogSocketRates();

    virtual void SetProperty(const int key, const std::string& value, const int slot = 0);
    virtual std::string GetProperty(const int key, const std::string& default_ = "", const int slot = 0);
    virtual void SetProperty(const int key, const int value, const int slot = 0);
    virtual int GetProperty(const int key, const int default_ = 0, const int slot = 0);

    virtual void SetTransport(FairMQTransportFactory* factory);

    virtual ~FairMQDevice();

  protected:
    std::string fId;

    int fNumIoThreads;

    int fNumInputs;
    int fNumOutputs;

    int fPortRangeMin;
    int fPortRangeMax;

    std::vector<std::string> fInputAddress;
    std::vector<std::string> fInputMethod;
    std::vector<std::string> fInputSocketType;
    std::vector<int> fInputSndBufSize;
    std::vector<int> fInputRcvBufSize;
    std::vector<int> fInputRateLogging;

    std::vector<std::string> fOutputAddress;
    std::vector<std::string> fOutputMethod;
    std::vector<std::string> fOutputSocketType;
    std::vector<int> fOutputSndBufSize;
    std::vector<int> fOutputRcvBufSize;
    std::vector<int> fOutputRateLogging;

    std::vector<FairMQSocket*>* fPayloadInputs;
    std::vector<FairMQSocket*>* fPayloadOutputs;

    int fLogIntervalInMs;

    FairMQTransportFactory* fTransportFactory;

    virtual void Init();
    virtual void Run();
    virtual void Pause();
    virtual void Shutdown();
    virtual void InitOutput();
    virtual void InitInput();
    virtual void Bind();
    virtual void Connect();

    virtual void Terminate();

  private:
    /// Copy Constructor
    FairMQDevice(const FairMQDevice&);
    FairMQDevice operator=(const FairMQDevice&);
};

#endif /* FAIRMQDEVICE_H_ */

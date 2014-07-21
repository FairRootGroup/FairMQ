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

using std::vector;
using std::cin;
using std::cout;
using std::endl;

class FairMQDevice : public FairMQStateMachine, public FairMQConfigurable
{
  public:
    enum
    {
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

    virtual void SetProperty(const int key, const string& value, const int slot = 0);
    virtual string GetProperty(const int key, const string& default_ = "", const int slot = 0);
    virtual void SetProperty(const int key, const int value, const int slot = 0);
    virtual int GetProperty(const int key, const int default_ = 0, const int slot = 0);

    virtual void SetTransport(FairMQTransportFactory* factory);

    virtual ~FairMQDevice();

  protected:
    string fId;
    int fNumIoThreads;

    int fNumInputs;
    int fNumOutputs;

    vector<string> fInputAddress;
    vector<string> fInputMethod;
    vector<string> fInputSocketType;
    vector<int> fInputSndBufSize;
    vector<int> fInputRcvBufSize;

    vector<string> fOutputAddress;
    vector<string> fOutputMethod;
    vector<string> fOutputSocketType;
    vector<int> fOutputSndBufSize;
    vector<int> fOutputRcvBufSize;

    vector<FairMQSocket*>* fPayloadInputs;
    vector<FairMQSocket*>* fPayloadOutputs;

    int fLogIntervalInMs;

    FairMQTransportFactory* fTransportFactory;

    virtual void Init();
    virtual void Run();
    virtual void Pause();
    virtual void Shutdown();
    virtual void InitOutput();
    virtual void InitInput();
};

#endif /* FAIRMQDEVICE_H_ */

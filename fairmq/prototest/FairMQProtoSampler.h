/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQProtoSampler.h
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQPROTOSAMPLER_H_
#define FAIRMQPROTOSAMPLER_H_

#include <string>

#include "FairMQDevice.h"

class FairMQProtoSampler : public FairMQDevice
{
  public:
    enum
    {
        EventRate = FairMQDevice::Last,
        EventSize,
        Last
    };
    FairMQProtoSampler();
    virtual ~FairMQProtoSampler();

    void ResetEventCounter();

    virtual void SetProperty(const int key, const std::string& value);
    virtual std::string GetProperty(const int key, const std::string& default_ = "");
    virtual void SetProperty(const int key, const int value);
    virtual int GetProperty(const int key, const int default_ = 0);

  protected:
    int fEventSize;
    int fEventRate;
    int fEventCounter;

    virtual void Run();
};

#endif /* FAIRMQPROTOSAMPLER_H_ */

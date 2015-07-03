/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQBinSampler.h
 *
 * @since 2014-02-24
 * @author A. Rybalchenko
 */

#ifndef FAIRMQBINSAMPLER_H_
#define FAIRMQBINSAMPLER_H_

#include <string>

#include "FairMQDevice.h"

struct Content
{
    double a;
    double b;
    int x;
    int y;
    int z;
};

class FairMQBinSampler : public FairMQDevice
{
  public:
    enum
    {
        EventRate = FairMQDevice::Last,
        EventSize,
        Last
    };
    FairMQBinSampler();
    virtual ~FairMQBinSampler();

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

#endif /* FAIRMQBINSAMPLER_H_ */

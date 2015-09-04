/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample1Sampler.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE1SAMPLER_H_
#define FAIRMQEXAMPLE1SAMPLER_H_

#include <string>

#include "FairMQDevice.h"

class FairMQExample1Sampler : public FairMQDevice
{
  public:
    enum
    {
        Text = FairMQDevice::Last,
        Last
    };
    FairMQExample1Sampler();
    virtual ~FairMQExample1Sampler();

    static void CustomCleanup(void* data, void* hint);

    virtual void SetProperty(const int key, const std::string& value);
    virtual std::string GetProperty(const int key, const std::string& default_ = "");
    virtual void SetProperty(const int key, const int value);
    virtual int GetProperty(const int key, const int default_ = 0);

  protected:
    std::string fText;

    virtual void Run();
};

#endif /* FAIRMQEXAMPLE1SAMPLER_H_ */

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample7Client.h
 *
 * @since 2015-10-26
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE7CLIENT_H_
#define FAIRMQEXAMPLE7CLIENT_H_

#include <string>

#include "FairMQDevice.h"

class FairMQExample7Client : public FairMQDevice
{
  public:
    enum
    {
        ParameterName = FairMQDevice::Last,
        Last
    };
    FairMQExample7Client();
    virtual ~FairMQExample7Client();

    static void CustomCleanup(void* data, void* hint);

    virtual void SetProperty(const int key, const std::string& value);
    virtual std::string GetProperty(const int key, const std::string& default_ = "");
    virtual void SetProperty(const int key, const int value);
    virtual int GetProperty(const int key, const int default_ = 0);

  protected:
    virtual void Run();

  private:
    int fRunId;
    std::string fParameterName;
};

#endif /* FAIRMQEXAMPLE7CLIENT_H_ */

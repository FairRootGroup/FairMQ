/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExampleClient.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLECLIENT_H_
#define FAIRMQEXAMPLECLIENT_H_

#include <string>

#include "FairMQDevice.h"

class FairMQExampleClient : public FairMQDevice
{
  public:
    enum
    {
        Text = FairMQDevice::Last,
        Last
    };
    FairMQExampleClient();
    virtual ~FairMQExampleClient();

    static void CustomCleanup(void *data, void* hint);

    virtual void SetProperty(const int key, const std::string& value, const int slot = 0);
    virtual std::string GetProperty(const int key, const std::string& default_ = "", const int slot = 0);
    virtual void SetProperty(const int key, const int value, const int slot = 0);
    virtual int GetProperty(const int key, const int default_ = 0, const int slot = 0);

  protected:
    std::string fText;

    virtual void Run();
};

#endif /* FAIRMQEXAMPLECLIENT_H_ */

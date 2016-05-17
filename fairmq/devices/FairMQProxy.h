/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQProxy.h
 *
 * @since 2013-10-02
 * @author A. Rybalchenko
 */

#ifndef FAIRMQPROXY_H_
#define FAIRMQPROXY_H_

#include "FairMQDevice.h"

class FairMQProxy : public FairMQDevice
{
  public:
    enum
    {
        Multipart = FairMQDevice::Last,
        Last
    };

    FairMQProxy();
    virtual ~FairMQProxy();

    virtual void SetProperty(const int key, const std::string& value);
    virtual std::string GetProperty(const int key, const std::string& default_ = "");
    virtual void SetProperty(const int key, const int value);
    virtual int GetProperty(const int key, const int default_ = 0);

    virtual std::string GetPropertyDescription(const int key);
    virtual void ListProperties();

  protected:
    int fMultipart;

    virtual void Run();
};

#endif /* FAIRMQPROXY_H_ */

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSink.h
 *
 * @since 2013-01-09
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSINK_H_
#define FAIRMQSINK_H_

#include <string>

#include "FairMQDevice.h"

class FairMQSink : public FairMQDevice
{
  public:
    enum
    {
        NumMsgs = FairMQDevice::Last,
        Last
    };

    FairMQSink();
    virtual ~FairMQSink();

    virtual void SetProperty(const int key, const std::string& value);
    virtual std::string GetProperty(const int key, const std::string& default_ = "");
    virtual void SetProperty(const int key, const int value);
    virtual int GetProperty(const int key, const int default_ = 0);

    virtual std::string GetPropertyDescription(const int key);
    virtual void ListProperties();

  protected:
    int fNumMsgs;

    virtual void Run();
};

#endif /* FAIRMQSINK_H_ */

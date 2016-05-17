/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSplitter.h
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSPLITTER_H_
#define FAIRMQSPLITTER_H_

#include "FairMQDevice.h"

class FairMQSplitter : public FairMQDevice
{
  public:
    enum
    {
        Multipart = FairMQDevice::Last,
        Last
    };

    FairMQSplitter();
    virtual ~FairMQSplitter();

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

#endif /* FAIRMQSPLITTER_H_ */

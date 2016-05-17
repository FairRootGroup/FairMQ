/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQMerger.h
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQMERGER_H_
#define FAIRMQMERGER_H_

#include "FairMQDevice.h"

class FairMQMerger : public FairMQDevice
{
  public:
    enum
    {
        Multipart = FairMQDevice::Last,
        Last
    };

    FairMQMerger();
    virtual ~FairMQMerger();

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

#endif /* FAIRMQMERGER_H_ */

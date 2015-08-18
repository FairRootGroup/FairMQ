/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample3Processor.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE3PROCESSOR_H_
#define FAIRMQEXAMPLE3PROCESSOR_H_

#include <string>

#include "FairMQDevice.h"

class FairMQExample3Processor : public FairMQDevice
{
  public:
    enum
    {
        Text = FairMQDevice::Last,
        TaskIndex,
        Last
    };
    FairMQExample3Processor();
    virtual ~FairMQExample3Processor();

    static void CustomCleanup(void* data, void* hint);

    virtual void SetProperty(const int key, const std::string& value);
    virtual std::string GetProperty(const int key, const std::string& default_ = "");
    virtual void SetProperty(const int key, const int value);
    virtual int GetProperty(const int key, const int default_ = 0);

  protected:
    int fTaskIndex;

    virtual void Run();
};

#endif /* FAIRMQEXAMPLE3PROCESSOR_H_ */

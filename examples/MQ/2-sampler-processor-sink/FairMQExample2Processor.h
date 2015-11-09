/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample2Processor.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE2PROCESSOR_H_
#define FAIRMQEXAMPLE2PROCESSOR_H_

#include <string>

#include "FairMQDevice.h"

class FairMQExample2Processor : public FairMQDevice
{
  public:
    enum
    {
        Text = FairMQDevice::Last,
        Last
    };
    FairMQExample2Processor();
    virtual ~FairMQExample2Processor();

    static void CustomCleanup(void* data, void* hint);

  protected:
    std::string fText;

    virtual void Run();
};

#endif /* FAIRMQEXAMPLE2PROCESSOR_H_ */

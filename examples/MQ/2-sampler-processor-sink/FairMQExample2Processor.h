/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQEXAMPLE2PROCESSOR_H_
#define FAIRMQEXAMPLE2PROCESSOR_H_

#include "FairMQDevice.h"

class FairMQExample2Processor : public FairMQDevice
{
  public:
    FairMQExample2Processor();
    virtual ~FairMQExample2Processor();

  protected:
    bool HandleData(FairMQMessagePtr&, int);
};

#endif /* FAIRMQEXAMPLE2PROCESSOR_H_ */

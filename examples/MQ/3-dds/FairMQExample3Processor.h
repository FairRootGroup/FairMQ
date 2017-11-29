/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQEXAMPLE2PROCESSOR_H_
#define FAIRMQEXAMPLE2PROCESSOR_H_

#include "FairMQDevice.h"

class FairMQExample3Processor : public FairMQDevice
{
  public:
    FairMQExample3Processor();
    virtual ~FairMQExample3Processor();

  protected:
    bool HandleData(FairMQMessagePtr&, int);
};

#endif /* FAIRMQEXAMPLE3PROCESSOR_H_ */

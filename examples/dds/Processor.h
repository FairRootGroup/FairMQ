/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQEXAMPLEDDSPROCESSOR_H
#define FAIRMQEXAMPLEDDSPROCESSOR_H

#include "FairMQDevice.h"

namespace example_dds
{

class Processor : public FairMQDevice
{
  public:
    Processor();
    virtual ~Processor();

  protected:
    bool HandleData(FairMQMessagePtr&, int);
};

}

#endif /* FAIRMQEXAMPLEDDSPROCESSOR_H */

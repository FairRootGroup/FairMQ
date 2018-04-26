/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQEXAMPLE1N1PROCESSOR_H_
#define FAIRMQEXAMPLE1N1PROCESSOR_H_

#include "FairMQDevice.h"

namespace example_1_n_1
{

class Processor : public FairMQDevice
{
  public:
    Processor();
    virtual ~Processor();

  protected:
    bool HandleData(FairMQMessagePtr&, int);
};

} // namespace example_1_n_1

#endif /* FAIRMQEXAMPLE1N1PROCESSOR_H_ */

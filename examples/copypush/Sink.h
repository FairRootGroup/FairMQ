/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Sink.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLECOPYPUSHSINK_H
#define FAIRMQEXAMPLECOPYPUSHSINK_H

#include "FairMQDevice.h"

#include <stdint.h> // uint64_t

namespace example_copypush
{

class Sink : public FairMQDevice
{
  public:
    Sink();
    virtual ~Sink();

  protected:
    virtual void InitTask();
    bool HandleData(FairMQMessagePtr&, int);

  private:
    uint64_t fMaxIterations;
    uint64_t fNumIterations;
};

} // namespace example_copypush

#endif /* FAIRMQEXAMPLECOPYPUSHSINK_H */

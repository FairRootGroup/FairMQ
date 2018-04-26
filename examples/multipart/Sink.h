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

#ifndef FAIRMQEXAMPLEMULTIPARTSINK_H
#define FAIRMQEXAMPLEMULTIPARTSINK_H

#include "FairMQDevice.h"

namespace example_multipart
{

class Sink : public FairMQDevice
{
  public:
    Sink();
    virtual ~Sink();

  protected:
    bool HandleData(FairMQParts&, int);
};

}
#endif /* FAIRMQEXAMPLEMULTIPARTSINK_H */

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQBuffer.h
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQBUFFER_H_
#define FAIRMQBUFFER_H_

#include "FairMQDevice.h"

class FairMQBuffer : public FairMQDevice
{
  public:
    FairMQBuffer();
    virtual ~FairMQBuffer();

  protected:
    virtual void Run();
};

#endif /* FAIRMQBUFFER_H_ */

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQProtoSink.h
 *
 * @since 2013-01-09
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQPROTOSINK_H_
#define FAIRMQPROTOSINK_H_

#include "FairMQDevice.h"

struct Content
{
    double a;
    double b;
    int x;
    int y;
    int z;
};

class FairMQProtoSink : public FairMQDevice
{
  public:
    FairMQProtoSink();
    virtual ~FairMQProtoSink();

  protected:
    virtual void Run();
};

#endif /* FAIRMQPROTOSINK_H_ */

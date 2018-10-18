/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSplitter.h
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#ifndef FAIRMQSPLITTER_H_
#define FAIRMQSPLITTER_H_

#include "FairMQDevice.h"

#include <string>

class FairMQSplitter : public FairMQDevice
{
  public:
    FairMQSplitter();
    virtual ~FairMQSplitter();

  protected:
    bool fMultipart;
    int fNumOutputs;
    int fDirection;
    std::string fInChannelName;
    std::string fOutChannelName;

    virtual void InitTask();

    bool HandleSingleData(std::unique_ptr<FairMQMessage>&, int);
    bool HandleMultipartData(FairMQParts&, int);
};

#endif /* FAIRMQSPLITTER_H_ */

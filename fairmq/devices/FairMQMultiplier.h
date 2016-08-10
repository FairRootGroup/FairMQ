/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQMULTIPLIER_H_
#define FAIRMQMULTIPLIER_H_

#include "FairMQDevice.h"

#include <string>

class FairMQMultiplier : public FairMQDevice
{
  public:
    FairMQMultiplier();
    virtual ~FairMQMultiplier();

  protected:
    int fMultipart;
    int fNumOutputs;
    std::string fInChannelName;
    std::vector<std::string> fOutChannelNames;

    virtual void InitTask();

    bool HandleSingleData(std::unique_ptr<FairMQMessage>&, int);
    bool HandleMultipartData(FairMQParts&, int);
};

#endif /* FAIRMQMULTIPLIER_H_ */

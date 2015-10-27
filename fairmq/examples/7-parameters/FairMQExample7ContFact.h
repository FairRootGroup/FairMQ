/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQEXAMPLE7CONTFACT_H_
#define FAIRMQEXAMPLE7CONTFACT_H_

#include "FairContFact.h"

class FairContainer;

class FairMQExample7ContFact : public FairContFact
{
  private:
    void setAllContainers();

  public:
    FairMQExample7ContFact();
    ~FairMQExample7ContFact() {}

    FairParSet* createContainer(FairContainer*);
    ClassDef(FairMQExample7ContFact,0)
};

#endif  /* FAIRMQEXAMPLE7CONTFACT_H_ */

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include <iostream>

#include "FairRuntimeDb.h"

#include "FairMQExample7ContFact.h"
#include "FairMQExample7ParOne.h"

static FairMQExample7ContFact gFairMQExample7ContFact;

FairMQExample7ContFact::FairMQExample7ContFact()
{
    fName = "FairMQExample7ContFact";
    fTitle = "Factory for parameter containers in FairMQ Example 7";
    setAllContainers();
    FairRuntimeDb::instance()->addContFactory(this);
}

void FairMQExample7ContFact::setAllContainers()
{
    FairContainer* container = new FairContainer("FairMQExample7ParOne", "FairMQExample7ParOne Parameters", "TestDefaultContext");
    container->addContext("TestNonDefaultContext");

    containers->Add(container);
}

FairParSet* FairMQExample7ContFact::createContainer(FairContainer* container)
{
    const char* name = container->GetName();
    FairParSet* p = NULL;

    if (strcmp(name, "FairMQExample7ParOne") == 0)
    {
        p = new FairMQExample7ParOne(container->getConcatName().Data(), container->GetTitle(), container->getContext());
    }

    return p;
}

ClassImp(FairMQExample7ContFact);

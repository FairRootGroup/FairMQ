/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "FairMQExample7ParOne.h"

#include "FairParamList.h"
#include "FairDetParIo.h"

#include "FairLogger.h"
#include "TString.h"

FairMQExample7ParOne::FairMQExample7ParOne(const char* name, const char* title, const char* context) :
    FairParGenericSet(name, title, context),
    fParameterValue(0)
{
    detName = "TutorialDet";
}

FairMQExample7ParOne::~FairMQExample7ParOne()
{
    clear();
}

void FairMQExample7ParOne::clear()
{
    status = kFALSE;
    resetInputVersions();
}

void FairMQExample7ParOne::print()
{
    LOG(INFO) << "Print" << FairLogger::endl;
    LOG(INFO) << "fParameterValue: " << fParameterValue << FairLogger::endl;
}

void FairMQExample7ParOne::putParams(FairParamList* list)
{
    LOG(INFO) << "FairMQExample7ParOne::putParams()" << FairLogger::endl;

    if (!list)
    {
        return;
    }

    list->add("Example7ParameterValue", fParameterValue);
}


Bool_t FairMQExample7ParOne::getParams(FairParamList* list)
{
    LOG(INFO) << "FairMQExample7ParOne::getParams()" << FairLogger::endl;

    if (!list)
    {
        return kFALSE;
    }

    if (!list->fill("Example7ParameterValue", &fParameterValue))
    {
        return kFALSE;
    }

    return kTRUE;
}

ClassImp(FairMQExample7ParOne)

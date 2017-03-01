/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTestPull.cxx
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include "FairMQTestPull.h"
#include "FairMQLogger.h"

FairMQTestPull::FairMQTestPull()
{
}

void FairMQTestPull::Run()
{
    FairMQMessagePtr msg(NewMessage());

    if (Receive(msg, "data") >= 0)
    {
        LOG(INFO) << "PUSH-PULL test successfull";
    }
}

FairMQTestPull::~FairMQTestPull()
{
}

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

#include <memory> // unique_ptr

#include "FairMQTestPull.h"
#include "FairMQLogger.h"

FairMQTestPull::FairMQTestPull()
{
}

void FairMQTestPull::Run()
{
    std::unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());

    if (fChannels.at("data").at(0).Receive(msg) >= 0)
    {
        LOG(INFO) << "PUSH-PULL test successfull";
    }
}

FairMQTestPull::~FairMQTestPull()
{
}

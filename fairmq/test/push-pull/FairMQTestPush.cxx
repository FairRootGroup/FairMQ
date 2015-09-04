/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQTestPush.cpp
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include <memory> // unique_ptr

#include "FairMQTestPush.h"
#include "FairMQLogger.h"

FairMQTestPush::FairMQTestPush()
{
}

void FairMQTestPush::Run()
{
    std::unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());
    fChannels.at("data").at(0).Send(msg);
}

FairMQTestPush::~FairMQTestPush()
{
}

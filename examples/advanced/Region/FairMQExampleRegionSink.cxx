/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExampleRegionSink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "FairMQExampleRegionSink.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExampleRegionSink::FairMQExampleRegionSink()
{
}

void FairMQExampleRegionSink::Run()
{
    FairMQChannel& dataInChannel = fChannels.at("data").at(0);

    while (CheckCurrentState(RUNNING))
    {
        FairMQMessagePtr msg(dataInChannel.Transport()->CreateMessage());
        dataInChannel.Receive(msg);
    }
}

FairMQExampleRegionSink::~FairMQExampleRegionSink()
{
}

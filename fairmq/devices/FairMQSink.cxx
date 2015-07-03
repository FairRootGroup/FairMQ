/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSink.cxx
 *
 * @since 2013-01-09
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQSink.h"
#include "FairMQLogger.h"

FairMQSink::FairMQSink()
{
}

void FairMQSink::Run()
{
    // store the channel reference to avoid traversing the map on every loop iteration
    const FairMQChannel& dataChannel = fChannels.at("data-in").at(0);

    while (CheckCurrentState(RUNNING))
    {
        FairMQMessage* msg = fTransportFactory->CreateMessage();

        dataChannel.Receive(msg);

        delete msg;
    }
}

FairMQSink::~FairMQSink()
{
}

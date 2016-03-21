/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSplitter.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQLogger.h"
#include "FairMQSplitter.h"

FairMQSplitter::FairMQSplitter()
{
}

FairMQSplitter::~FairMQSplitter()
{
}

void FairMQSplitter::Run()
{
    int numOutputs = fChannels.at("data-out").size();

    int direction = 0;

    while (CheckCurrentState(RUNNING))
    {
        FairMQParts parts;

        if (Receive(parts, "data-in") >= 0)
        {
            if (Send(parts, "data-out", direction) < 0)
            {
                LOG(DEBUG) << "Transfer interrupted";
                break;
            }
        }
        else
        {
            LOG(DEBUG) << "Transfer interrupted";
            break;
        }

        ++direction;
        if (direction >= numOutputs)
        {
            direction = 0;
        }
    }
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *              GNU Lesser General Public Licence (LGPL) version 3,             *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQSplitter.cxx
 *
 * @since 2012-12-06
 * @author D. Klein, A. Rybalchenko
 */

#include "FairMQSplitter.h"

#include "../FairMQLogger.h"
#include "../options/FairMQProgOptions.h"

using namespace std;

FairMQSplitter::FairMQSplitter()
    : fMultipart(true)
    , fNumOutputs(0)
    , fDirection(0)
    , fInChannelName()
    , fOutChannelName()
{
}

FairMQSplitter::~FairMQSplitter()
{
}

void FairMQSplitter::InitTask()
{
    fMultipart = fConfig->GetValue<bool>("multipart");
    fInChannelName = fConfig->GetValue<string>("in-channel");
    fOutChannelName = fConfig->GetValue<string>("out-channel");
    fNumOutputs = fChannels.at(fOutChannelName).size();
    fDirection = 0;

    if (fMultipart)
    {
        OnData(fInChannelName, &FairMQSplitter::HandleMultipartData);
    }
    else
    {
        OnData(fInChannelName, &FairMQSplitter::HandleSingleData);
    }
}

bool FairMQSplitter::HandleSingleData(FairMQMessagePtr& payload, int /*index*/)
{
    Send(payload, fOutChannelName, fDirection);

    if (++fDirection >= fNumOutputs)
    {
        fDirection = 0;
    }

    return true;
}

bool FairMQSplitter::HandleMultipartData(FairMQParts& payload, int /*index*/)
{
    Send(payload, fOutChannelName, fDirection);

    if (++fDirection >= fNumOutputs)
    {
        fDirection = 0;
    }

    return true;
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample3Sampler.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQExample3Sampler.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample3Sampler::FairMQExample3Sampler()
{
}

void FairMQExample3Sampler::CustomCleanup(void *data, void *object)
{
    delete (string*)object;
}

void FairMQExample3Sampler::Run()
{
    while (CheckCurrentState(RUNNING))
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        string* text = new string("Data");

        unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text));

        LOG(INFO) << "Sending \"Data\"";

        fChannels.at("data-out").at(0).Send(msg);
    }
}

FairMQExample3Sampler::~FairMQExample3Sampler()
{
}

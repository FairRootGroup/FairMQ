/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample6Broadcaster.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <memory> // unique_ptr
#include <string>

#include <boost/thread.hpp>

#include "FairMQExample6Broadcaster.h"
#include "FairMQLogger.h"

using namespace std;

FairMQExample6Broadcaster::FairMQExample6Broadcaster()
{
}

void FairMQExample6Broadcaster::CustomCleanup(void *data, void *object)
{
    delete (string*)object;
}

void FairMQExample6Broadcaster::Run()
{
    while (CheckCurrentState(RUNNING))
    {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

        string* text = new string("OK");
        unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage(const_cast<char*>(text->c_str()), text->length(), CustomCleanup, text));
        LOG(INFO) << "Sending \"" << "OK" << "\"";
        fChannels.at("broadcast-out").at(0).Send(msg);
    }
}

FairMQExample6Broadcaster::~FairMQExample6Broadcaster()
{
}

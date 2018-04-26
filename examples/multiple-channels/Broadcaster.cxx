/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Broadcaster.cpp
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include <thread> // this_thread::sleep_for
#include <chrono>

#include "Broadcaster.h"

using namespace std;

namespace example_multiple_channels
{

Broadcaster::Broadcaster()
{
}

bool Broadcaster::ConditionalRun()
{
    this_thread::sleep_for(chrono::seconds(1));

    // NewSimpleMessage creates a copy of the data and takes care of its destruction (after the transfer takes place).
    // Should only be used for small data because of the cost of an additional copy
    FairMQMessagePtr msg(NewSimpleMessage("OK"));

    LOG(info) << "Sending OK";

    Send(msg, "broadcast");

    return true;
}

Broadcaster::~Broadcaster()
{
}

}

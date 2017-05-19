/********************************************************************************
 * Copyright (C) 2015-2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "devices/TestPollIn.cxx"
#include "devices/TestPollOut.cxx"
#include "devices/TestPub.cxx"
#include "devices/TestPull.cxx"
#include "devices/TestPush.cxx"
#include "devices/TestRep.cxx"
#include "devices/TestReq.cxx"
#include "devices/TestSub.cxx"
#include "devices/TestTransferTimeout.cxx"
#include <boost/program_options.hpp>
#include <iostream>
#include <runFairMQDevice.h>
#include <string>

namespace bpo = boost::program_options;

auto addCustomOptions(bpo::options_description& options) -> void
{
    options.add_options()
        ("poll-type", bpo::value<int>()->default_value(0), "Poll type switch(0 - vector of (sub-)channels, 1 - vector of channel names)");
}

auto getDevice(const FairMQProgOptions& config) -> FairMQDevicePtr
{
    using namespace std;
    using namespace fair::mq::test;

    auto id = config.GetValue<std::string>("id");
    if (0 == id.find("pull_"))
    {
        return new Pull;
    }
    else if (0 == id.find("push_"))
    {
        return new Push;
    }
    else if (0 == id.find("sub_"))
    {
        return new Sub;
    }
    else if (0 == id.find("pub_"))
    {
        return new Pub;
    }
    else if (0 == id.find("req_"))
    {
        return new Req;
    }
    else if (0 == id.find("rep_"))
    {
        return new Rep;
    }
    else if (0 == id.find("transfer_timeout_"))
    {
        return new TransferTimeout;
    }
    else if (0 == id.find("pollout_"))
    {
        return new PollOut;
    }
    else if (0 == id.find("pollin_"))
    {
        return new PollIn;
    }
    else
    {
        cerr << "Don't know id '" << id << "'" << endl;
        return nullptr;
    }
}

/********************************************************************************
 * Copyright (C) 2015-2022 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "devices/TestPairLeft.h"
#include "devices/TestPairRight.h"
#include "devices/TestPollIn.h"
#include "devices/TestPollOut.h"
#include "devices/TestPub.h"
#include "devices/TestPull.h"
#include "devices/TestPush.h"
#include "devices/TestRep.h"
#include "devices/TestReq.h"
#include "devices/TestSub.h"
#include "devices/TestTransferTimeout.h"
#include "devices/TestWaitFor.h"
#include "devices/TestExceptions.h"
#include "devices/TestErrorState.h"
#include "devices/TestSignals.h"

#include <fairmq/runDevice.h>

#include <boost/program_options.hpp>
#include <iostream>
#include <string>

namespace bpo = boost::program_options;

auto addCustomOptions(bpo::options_description& options) -> void
{
    options.add_options()
        ("poll-type", bpo::value<int>()->default_value(0), "Poll type switch(0 - vector of (sub-)channels, 1 - vector of channel names)");
}

auto getDevice(fair::mq::ProgOptions& config) -> std::unique_ptr<fair::mq::Device>
{
    using namespace std;
    using namespace fair::mq::test;

    auto id = config.GetProperty<std::string>("id");

    if (0 == id.find("pull_")) {
        return std::make_unique<Pull>();
    } else if (0 == id.find("push_")) {
        return std::make_unique<Push>();
    } else if (0 == id.find("sub_")) {
        return std::make_unique<Sub>();
    } else if (0 == id.find("pub_")) {
        return std::make_unique<Pub>();
    } else if (0 == id.find("req_")) {
        return std::make_unique<Req>();
    } else if (0 == id.find("rep_")) {
        return std::make_unique<Rep>();
    } else if (0 == id.find("transfer_timeout_")) {
        return std::make_unique<TransferTimeout>();
    } else if (0 == id.find("pollout_")) {
        return std::make_unique<PollOut>();
    } else if (0 == id.find("pollin_")) {
        return std::make_unique<PollIn>();
    } else if (0 == id.find("pairleft_")) {
        return std::make_unique<PairLeft>();
    } else if (0 == id.find("pairright_")) {
        return std::make_unique<PairRight>();
    } else if (0 == id.find("waitfor_")) {
        return std::make_unique<TestWaitFor>();
    } else if (0 == id.find("exceptions_")) {
        return std::make_unique<Exceptions>();
    } else if (0 == id.find("error_state_")) {
        return std::make_unique<ErrorState>();
    } else if (0 == id.find("signals_")) {
        return std::make_unique<Signals>();
    } else {
        cerr << "Don't know id '" << id << "'" << endl;
        return nullptr;
    }
}

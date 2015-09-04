/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runTestReq.cxx
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include <iostream>

#include "FairMQLogger.h"
#include "FairMQTestReq.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

int main(int argc, char** argv)
{
    FairMQTestReq testReq;
    testReq.CatchSignals();

#ifdef NANOMSG
    testReq.SetTransport(new FairMQTransportFactoryNN());
#else
    testReq.SetTransport(new FairMQTransportFactoryZMQ());
#endif

    testReq.SetProperty(FairMQTestReq::Id, "testReq");

    FairMQChannel reqChannel("req", "bind", "tcp://127.0.0.1:5558");
    testReq.fChannels["data"].push_back(reqChannel);

    testReq.ChangeState("INIT_DEVICE");
    testReq.WaitForEndOfState("INIT_DEVICE");

    testReq.ChangeState("INIT_TASK");
    testReq.WaitForEndOfState("INIT_TASK");

    testReq.ChangeState("RUN");
    testReq.WaitForEndOfState("RUN");

    testReq.ChangeState("RESET_TASK");
    testReq.WaitForEndOfState("RESET_TASK");

    testReq.ChangeState("RESET_DEVICE");
    testReq.WaitForEndOfState("RESET_DEVICE");

    testReq.ChangeState("END");

    return 0;
}

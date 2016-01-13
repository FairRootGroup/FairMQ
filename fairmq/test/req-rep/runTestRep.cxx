/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runTestRep.cxx
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include <iostream>

#include "FairMQLogger.h"
#include "FairMQTestRep.h"

int main(int argc, char** argv)
{
    FairMQTestRep testRep;
    testRep.CatchSignals();
    testRep.SetTransport("zeromq");

    testRep.SetProperty(FairMQTestRep::Id, "testRep");

    FairMQChannel repChannel("rep", "connect", "tcp://127.0.0.1:5558");
    testRep.fChannels["data"].push_back(repChannel);

    testRep.ChangeState("INIT_DEVICE");
    testRep.WaitForEndOfState("INIT_DEVICE");

    testRep.ChangeState("INIT_TASK");
    testRep.WaitForEndOfState("INIT_TASK");

    testRep.ChangeState("RUN");
    testRep.WaitForEndOfState("RUN");

    testRep.ChangeState("RESET_TASK");
    testRep.WaitForEndOfState("RESET_TASK");

    testRep.ChangeState("RESET_DEVICE");
    testRep.WaitForEndOfState("RESET_DEVICE");

    testRep.ChangeState("END");

    return 0;
}

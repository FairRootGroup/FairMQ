/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQStateMachine.cxx
 *
 * @since 2012-10-25
 * @author D. Klein, A. Rybalchenko
 */

#include "FairMQStateMachine.h"
#include "FairMQLogger.h"

FairMQStateMachine::FairMQStateMachine()
{
    start();
}

FairMQStateMachine::~FairMQStateMachine()
{
    stop();
}

void FairMQStateMachine::ChangeState(int event)
{
    switch (event)
    {
        case INIT:
            process_event(FairMQFSM::INIT());
            return;
        case SETOUTPUT:
            process_event(FairMQFSM::SETOUTPUT());
            return;
        case SETINPUT:
            process_event(FairMQFSM::SETINPUT());
            return;
        case RUN:
            process_event(FairMQFSM::RUN());
            return;
        case PAUSE:
            process_event(FairMQFSM::PAUSE());
            return;
        case STOP:
            process_event(FairMQFSM::STOP());
            return;
        case END:
            process_event(FairMQFSM::END());
            return;
    }
}

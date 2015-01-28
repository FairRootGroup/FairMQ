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
    : fRunningFinished(false)
    , fRunningCondition()
    , fRunningMutex()
{
    start();
}

FairMQStateMachine::~FairMQStateMachine()
{
    stop();
}

int FairMQStateMachine::GetInterfaceVersion()
{
    return FAIRMQ_INTERFACE_VERSION;
}

bool FairMQStateMachine::ChangeState(int event)
{
    try
    {
        switch (event)
        {
            case INIT:
                process_event(FairMQFSM::INIT());
                return true;
            case SETOUTPUT:
                process_event(FairMQFSM::SETOUTPUT());
                return true;
            case SETINPUT:
                process_event(FairMQFSM::SETINPUT());
                return true;
            case BIND:
                process_event(FairMQFSM::BIND());
                return true;
            case CONNECT:
                process_event(FairMQFSM::CONNECT());
                return true;
            case RUN:
                process_event(FairMQFSM::RUN());
                return true;
            case PAUSE:
                process_event(FairMQFSM::PAUSE());
                return true;
            case STOP:
                process_event(FairMQFSM::STOP());
                return true;
            case END:
                process_event(FairMQFSM::END());
                return true;
            default:
                LOG(ERROR) << "Requested unsupported state: " << event;
                LOG(ERROR) << "Supported are: INIT, SETOUTPUT, SETINPUT, BIND, CONNECT, RUN, PAUSE, STOP, END";
                return false;
        }
    }
    catch (boost::bad_function_call& e)
    {
        LOG(ERROR) << e.what();
    }
}

bool FairMQStateMachine::ChangeState(std::string event)
{
    if (event == "INIT")
    {
        return ChangeState(INIT);
    }
    else if (event == "SETOUTPUT")
    {
        return ChangeState(SETOUTPUT);
    }
    else if (event == "SETINPUT")
    {
        return ChangeState(SETINPUT);
    }
    else if (event == "BIND")
    {
        return ChangeState(BIND);
    }
    else if (event == "CONNECT")
    {
        return ChangeState(CONNECT);
    }
    else if (event == "RUN")
    {
        return ChangeState(RUN);
    }
    else if (event == "PAUSE")
    {
        return ChangeState(PAUSE);
    }
    else if (event == "STOP")
    {
        return ChangeState(STOP);
    }
    else if (event == "END")
    {
        return ChangeState(END);
    }
    else
    {
        LOG(ERROR) << "Requested unsupported state: " << event;
        LOG(ERROR) << "Supported are: INIT, SETOUTPUT, SETINPUT, BIND, CONNECT, RUN, PAUSE, STOP, END";
        return false;
    }
}

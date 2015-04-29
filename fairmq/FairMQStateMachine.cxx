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
    : fInitializingFinished(false)
    , fInitializingCondition()
    , fInitializingMutex()
    , fInitializingTaskFinished(false)
    , fInitializingTaskCondition()
    , fInitializingTaskMutex()
    , fRunningFinished(false)
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
            case INIT_DEVICE:
                process_event(FairMQFSM::INIT_DEVICE());
                return true;
            case DEVICE_READY:
                process_event(FairMQFSM::DEVICE_READY());
                return true;
            case INIT_TASK:
                process_event(FairMQFSM::INIT_TASK());
                return true;
            case READY:
                process_event(FairMQFSM::READY());
                return true;
            case RUN:
                process_event(FairMQFSM::RUN());
                return true;
            case PAUSE:
                process_event(FairMQFSM::PAUSE());
                return true;
            case RESUME:
                process_event(FairMQFSM::RESUME());
                return true;
            case STOP:
                process_event(FairMQFSM::STOP());
                return true;
            case RESET_DEVICE:
                process_event(FairMQFSM::RESET_DEVICE());
                return true;
            case RESET_TASK:
                process_event(FairMQFSM::RESET_TASK());
                return true;
            case IDLE:
                process_event(FairMQFSM::IDLE());
                return true;
            case END:
                process_event(FairMQFSM::END());
                return true;
            default:
                LOG(ERROR) << "Requested unsupported state: " << event << std::endl
                           << "Supported are: INIT_DEVICE, INIT_TASK, RUN, PAUSE, RESUME, STOP, RESET_TASK, RESET_DEVICE, END";
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
    if (event == "INIT_DEVICE")
    {
        return ChangeState(INIT_DEVICE);
    }
    if (event == "DEVICE_READY")
    {
        return ChangeState(DEVICE_READY);
    }
    if (event == "INIT_TASK")
    {
        return ChangeState(INIT_TASK);
    }
    if (event == "READY")
    {
        return ChangeState(READY);
    }
    else if (event == "RUN")
    {
        return ChangeState(RUN);
    }
    else if (event == "PAUSE")
    {
        return ChangeState(PAUSE);
    }
    else if (event == "RESUME")
    {
        return ChangeState(RESUME);
    }
    else if (event == "STOP")
    {
        return ChangeState(STOP);
    }
    if (event == "RESET_DEVICE")
    {
        return ChangeState(RESET_DEVICE);
    }
    if (event == "RESET_TASK")
    {
        return ChangeState(RESET_TASK);
    }
    else if (event == "IDLE")
    {
        return ChangeState(IDLE);
    }
    else if (event == "END")
    {
        return ChangeState(END);
    }
    else
    {
        LOG(ERROR) << "Requested unsupported state: " << event << std::endl
                   << "Supported are: INIT_DEVICE, INIT_TASK, RUN, PAUSE, RESUME, STOP, RESET_TASK, RESET_DEVICE, END";
        return false;
    }
}

void FairMQStateMachine::WaitForEndOfState(int event)
{
    switch (event)
    {
        case INIT_DEVICE:
        {
            boost::unique_lock<boost::mutex> lock(fInitializingMutex);
            while (!fInitializingFinished)
            {
                fInitializingCondition.wait(lock);
            }
            break;
        }
        case INIT_TASK:
        {
            boost::unique_lock<boost::mutex> initTaskLock(fInitializingTaskMutex);
            while (!fInitializingTaskFinished)
            {
                fInitializingTaskCondition.wait(initTaskLock);
            }
            break;
        }
        case RUN:
        {
            boost::unique_lock<boost::mutex> runLock(fRunningMutex);
            while (!fRunningFinished)
            {
                fRunningCondition.wait(runLock);
            }
            break;
        }
        case RESET_TASK:
        {
            boost::unique_lock<boost::mutex> runLock(fResetTaskMutex);
            while (!fResetTaskFinished)
            {
                fResetTaskCondition.wait(runLock);
            }
            break;
        }
        case RESET_DEVICE:
        {
            boost::unique_lock<boost::mutex> runLock(fResetMutex);
            while (!fResetFinished)
            {
                fResetCondition.wait(runLock);
            }
            break;
        }
        default:
            LOG(ERROR) << "Requested state is either synchronous or does not exist.";
            break;
    }
}

void FairMQStateMachine::WaitForEndOfState(std::string event)
{
    if (event == "INIT_DEVICE")
    {
        return WaitForEndOfState(INIT_DEVICE);
    }
    if (event == "DEVICE_READY")
    {
        return WaitForEndOfState(DEVICE_READY);
    }
    if (event == "INIT_TASK")
    {
        return WaitForEndOfState(INIT_TASK);
    }
    if (event == "READY")
    {
        return WaitForEndOfState(READY);
    }
    else if (event == "RUN")
    {
        return WaitForEndOfState(RUN);
    }
    else if (event == "PAUSE")
    {
        return WaitForEndOfState(PAUSE);
    }
    else if (event == "RESUME")
    {
        return WaitForEndOfState(RESUME);
    }
    else if (event == "STOP")
    {
        return WaitForEndOfState(STOP);
    }
    if (event == "RESET_DEVICE")
    {
        return WaitForEndOfState(RESET_DEVICE);
    }
    if (event == "RESET_TASK")
    {
        return WaitForEndOfState(RESET_TASK);
    }
    else if (event == "IDLE")
    {
        return WaitForEndOfState(IDLE);
    }
    else if (event == "END")
    {
        return WaitForEndOfState(END);
    }
    else
    {
        LOG(ERROR) << "Requested unsupported state: " << event << std::endl
                   << "Supported are: INIT_DEVICE, INIT_TASK, RUN, PAUSE, RESUME, STOP, RESET_TASK, RESET_DEVICE, END";
    }
}
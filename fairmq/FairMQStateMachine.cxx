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


#include <boost/exception/all.hpp>
#include <boost/chrono.hpp> // for WaitForEndOfStateForMs()

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

int FairMQStateMachine::GetInterfaceVersion()
{
    return FAIRMQ_INTERFACE_VERSION;
}

int FairMQStateMachine::GetEventNumber(std::string event)
{
    if (event == "INIT_DEVICE") return INIT_DEVICE;
    if (event == "INIT_TASK") return INIT_TASK;
    if (event == "RUN") return RUN;
    if (event == "PAUSE") return PAUSE;
    if (event == "RESUME") return RESUME;
    if (event == "STOP") return STOP;
    if (event == "RESET_DEVICE") return RESET_DEVICE;
    if (event == "RESET_TASK") return RESET_TASK;
    if (event == "END") return END;
    LOG(ERROR) << "Requested number for non-existent event... " << event << std::endl
               << "Supported are: INIT_DEVICE, INIT_TASK, RUN, PAUSE, RESUME, STOP, RESET_DEVICE, RESET_TASK, END";
    return -1;
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
            case internal_DEVICE_READY:
                process_event(FairMQFSM::internal_DEVICE_READY());
                return true;
            case INIT_TASK:
                process_event(FairMQFSM::INIT_TASK());
                return true;
            case internal_READY:
                process_event(FairMQFSM::internal_READY());
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
            case internal_IDLE:
                process_event(FairMQFSM::internal_IDLE());
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
    catch (boost::thread_interrupted& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
    catch (boost::exception& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}

bool FairMQStateMachine::ChangeState(std::string event)
{
    return ChangeState(GetEventNumber(event));
}

void FairMQStateMachine::WaitForEndOfState(int event)
{
    try
    {
        switch (event)
        {
            case INIT_DEVICE:
            case INIT_TASK:
            case RUN:
            case RESET_TASK:
            case RESET_DEVICE:
            {
                try
                {
                    boost::unique_lock<boost::mutex> lock(fStateMutex);
                    while (!fStateFinished)
                    {
                        fStateCondition.wait(lock);
                    }
                }
                catch (boost::exception& e)
                {
                    LOG(ERROR) << boost::diagnostic_information(e);
                }
                break;
            }
            default:
                LOG(ERROR) << "Requested state is either synchronous or does not exist.";
                break;
        }
    }
    catch (boost::thread_interrupted& e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}

void FairMQStateMachine::WaitForEndOfState(std::string event)
{
    return WaitForEndOfState(GetEventNumber(event));
}

bool FairMQStateMachine::WaitForEndOfStateForMs(int event, int durationInMs)
{
    try
    {
        switch (event)
        {
            case INIT_DEVICE:
            case INIT_TASK:
            case RUN:
            case RESET_TASK:
            case RESET_DEVICE:
            {
                boost::unique_lock<boost::mutex> lock(fStateMutex);
                while (!fStateFinished)
                {
                    fStateCondition.wait_until(lock, boost::chrono::system_clock::now() + boost::chrono::milliseconds(durationInMs));
                    if (!fStateFinished)
                    {
                        return false;
                    }
                }
                return true;
                break;
            }
            default:
                LOG(ERROR) << "Requested state is either synchronous or does not exist.";
                return false;
        }
    }
    catch (boost::thread_interrupted &e)
    {
        LOG(ERROR) << boost::diagnostic_information(e);
    }
}

bool FairMQStateMachine::WaitForEndOfStateForMs(std::string event, int durationInMs)
{
    return WaitForEndOfStateForMs(GetEventNumber(event), durationInMs);
}

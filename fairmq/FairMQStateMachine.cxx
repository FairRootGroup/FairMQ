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

#include <chrono> // WaitForEndOfStateForMs()

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
    if (event == "STOP") return STOP;
    if (event == "RESET_DEVICE") return RESET_DEVICE;
    if (event == "RESET_TASK") return RESET_TASK;
    if (event == "END") return END;
    if (event == "ERROR_FOUND") return ERROR_FOUND;
    LOG(ERROR) << "Requested number for non-existent event... " << event << std::endl
               << "Supported are: INIT_DEVICE, INIT_TASK, RUN, PAUSE, STOP, RESET_DEVICE, RESET_TASK, END, ERROR_FOUND";
    return -1;
}

bool FairMQStateMachine::ChangeState(int event)
{
    try
    {
        switch (event)
        {
            case INIT_DEVICE:
            {
                std::lock_guard<std::mutex> lock(fChangeStateMutex);
                process_event(FairMQFSM::INIT_DEVICE());
                return true;
            }
            case internal_DEVICE_READY:
            {
                std::lock_guard<std::mutex> lock(fChangeStateMutex);
                process_event(FairMQFSM::internal_DEVICE_READY());
                return true;
            }
            case INIT_TASK:
            {
                std::lock_guard<std::mutex> lock(fChangeStateMutex);
                process_event(FairMQFSM::INIT_TASK());
                return true;
            }
            case internal_READY:
            {
                // std::lock_guard<std::mutex> lock(fChangeStateMutex); // InitTask is synchronous, until ROOT workaround is no longer needed.
                process_event(FairMQFSM::internal_READY());
                return true;
            }
            case RUN:
            {
                std::lock_guard<std::mutex> lock(fChangeStateMutex);
                process_event(FairMQFSM::RUN());
                return true;
            }
            case PAUSE:
            {
                std::lock_guard<std::mutex> lock(fChangeStateMutex);
                process_event(FairMQFSM::PAUSE());
                return true;
            }
            case STOP:
            {
                std::lock_guard<std::mutex> lock(fChangeStateMutex);
                process_event(FairMQFSM::STOP());
                return true;
            }
            case RESET_DEVICE:
            {
                std::lock_guard<std::mutex> lock(fChangeStateMutex);
                process_event(FairMQFSM::RESET_DEVICE());
                return true;
            }
            case RESET_TASK:
            {
                std::lock_guard<std::mutex> lock(fChangeStateMutex);
                process_event(FairMQFSM::RESET_TASK());
                return true;
            }
            case internal_IDLE:
            {
                std::lock_guard<std::mutex> lock(fChangeStateMutex);
                process_event(FairMQFSM::internal_IDLE());
                return true;
            }
            case END:
            {
                std::lock_guard<std::mutex> lock(fChangeStateMutex);
                process_event(FairMQFSM::END());
                return true;
            }
            case ERROR_FOUND:
            {
                std::lock_guard<std::mutex> lock(fChangeStateMutex);
                process_event(FairMQFSM::ERROR_FOUND());
                return true;
            }
            default:
            {
                LOG(ERROR) << "Requested state transition with an unsupported event: " << event << std::endl
                           << "Supported are: INIT_DEVICE, INIT_TASK, RUN, PAUSE, STOP, RESET_TASK, RESET_DEVICE, END, ERROR_FOUND";
                return false;
            }
        }
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Exception in FairMQStateMachine::ChangeState(): " << e.what();
        exit(EXIT_FAILURE);
    }
    return false;
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
            case RUN:
            case RESET_TASK:
            case RESET_DEVICE:
            {
                std::unique_lock<std::mutex> lock(fWorkMutex);
                while (fWorkActive || fWorkAvailable)
                {
                    fWorkDoneCondition.wait_for(lock, std::chrono::seconds(1));
                }

                break;
            }
            case INIT_TASK:
                break; // InitTask is synchronous, until ROOT workaround is no longer needed.
            default:
                LOG(ERROR) << "Requested state is either synchronous or does not exist.";
                break;
        }
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Exception in FairMQStateMachine::WaitForEndOfState(): " << e.what();
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
            case RUN:
            case RESET_TASK:
            case RESET_DEVICE:
            {
                std::unique_lock<std::mutex> lock(fWorkMutex);
                while (fWorkActive || fWorkAvailable)
                {
                    fWorkDoneCondition.wait_for(lock, std::chrono::milliseconds(durationInMs));
                    if (fWorkActive)
                    {
                        return false;
                    }
                }
                return true;
                break;
            }
            case INIT_TASK:
                break; // InitTask is synchronous, until ROOT workaround is no longer needed.
            default:
                LOG(ERROR) << "Requested state is either synchronous or does not exist.";
                return false;
        }
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Exception in FairMQStateMachine::WaitForEndOfStateForMs(): " << e.what();
    }
    return false;
}

bool FairMQStateMachine::WaitForEndOfStateForMs(std::string event, int durationInMs)
{
    return WaitForEndOfStateForMs(GetEventNumber(event), durationInMs);
}

void FairMQStateMachine::OnStateChange(const std::string& key, std::function<void(const State)> callback)
{
    fStateChangeCallbacksMap.insert({key, fStateChangeCallback.connect(callback)});
}

void FairMQStateMachine::UnsubscribeFromStateChange(const std::string& key)
{
    fStateChangeCallbacksMap.at(key).disconnect();
    //fStateChangeCallbacksMap.erase(key);
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "dds_intercom.h"

#include "FairMQControlPlugin.h"

#include "FairMQLogger.h"
#include "FairMQDevice.h"

#include <string>
#include <exception>
#include <condition_variable>
#include <mutex>
#include <chrono>

using namespace std;
using namespace dds::intercom_api;

class FairMQControlPluginDDS
{
  public:
    static FairMQControlPluginDDS* GetInstance()
    {
        if (fInstance == NULL)
        {
            fInstance  = new FairMQControlPluginDDS();
        }
        return fInstance;
    }

   static void ResetInstance()
   {
        try
        {
            delete fInstance;
            fInstance = NULL;
        }
        catch (exception& e)
        {
            LOG(ERROR) << "Error: " << e.what() << endl;
            return;
        }
   }

    void Init(FairMQDevice& device)
    {
        string id = device.GetProperty(FairMQDevice::Id, "");
        string pid(to_string(getpid()));

        try
        {
            fDDSCustomCmd.subscribe([id, pid, &device, this](const string& cmd, const string& cond, uint64_t senderId)
            {
                LOG(INFO) << "Received command: " << cmd;

                if (cmd == "check-state")
                {
                    fDDSCustomCmd.send(id + ": " + device.GetCurrentStateName() + " (pid: " + pid + ")", to_string(senderId));
                }
                else if (fEvents.find(cmd) != fEvents.end())
                {
                    fDDSCustomCmd.send(id + ": attempting to " + cmd, to_string(senderId));
                    device.ChangeState(cmd);
                }
                else if (cmd == "END")
                {
                    fDDSCustomCmd.send(id + ": attempting to " + cmd, to_string(senderId));
                    device.ChangeState(cmd);
                    fDDSCustomCmd.send(id + ": " + device.GetCurrentStateName(), to_string(senderId));
                    if (device.GetCurrentStateName() == "EXITING")
                    {
                        unique_lock<mutex> lock(fMtx);
                        fStopCondition.notify_one();
                    }
                }
                else
                {
                    LOG(WARN) << "Unknown command: " << cmd;
                    LOG(WARN) << "Origin: " << senderId;
                    LOG(WARN) << "Destination: " << cond;
                }
            });
        }
        catch (exception& e)
        {
            LOG(ERROR) << "Error: " << e.what() << endl;
            return;
        }
    }

    void Run(FairMQDevice& device)
    {
        try
        {
            fService.start();

            LOG(INFO) << "Listening for commands from DDS...";
            unique_lock<mutex> lock(fMtx);
            while (!device.Terminated())
            {
                fStopCondition.wait_for(lock, chrono::seconds(1));
            }
            LOG(DEBUG) << "Stopping DDS control plugin";
        }
        catch (exception& e)
        {
            LOG(ERROR) << "Error: " << e.what() << endl;
            return;
        }
    }

  private:
    FairMQControlPluginDDS()
        : fService()
        , fDDSCustomCmd(fService)
        , fMtx()
        , fStopCondition()
        , fEvents({ "INIT_DEVICE", "INIT_TASK", "PAUSE", "RUN", "STOP", "RESET_TASK", "RESET_DEVICE" })
    {
        fService.subscribeOnError([](const EErrorCode errorCode, const string& errorMsg) {
            LOG(ERROR) << "Error received: error code: " << errorCode << ", error message: " << errorMsg << endl;
        });
    }

    static FairMQControlPluginDDS* fInstance;

    CIntercomService fService;
    CCustomCmd fDDSCustomCmd;

    mutex fMtx;
    condition_variable fStopCondition;

    const set<string> fEvents;
};

FairMQControlPluginDDS* FairMQControlPluginDDS::fInstance = NULL;

void initControl(FairMQDevice& device)
{
    FairMQControlPluginDDS::GetInstance()->Init(device);
}

/// Controls device state via DDS custom commands interface
/// \param device  Reference to FairMQDevice whose state to control
void handleStateChanges(FairMQDevice& device)
{
    FairMQControlPluginDDS::GetInstance()->Run(device);
}

void stopControl()
{
    LOG(DEBUG) << "[FairMQControlPluginDDS]: " << "Resetting instance.";
    FairMQControlPluginDDS::ResetInstance();
    LOG(DEBUG) << "[FairMQControlPluginDDS]: " << "Instance has been reset.";
}

FairMQControlPlugin fairmqControlPlugin = { initControl, handleStateChanges, stopControl };

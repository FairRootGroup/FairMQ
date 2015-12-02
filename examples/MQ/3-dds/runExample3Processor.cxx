/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runExample2Processor.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <mutex>
#include <condition_variable>

#include "boost/program_options.hpp"
#include <boost/asio.hpp> // for DDS

#include "FairMQLogger.h"
#include "FairMQProgOptions.h"
#include "FairMQExample3Processor.h"
#include "FairMQTools.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

#include "KeyValue.h" // DDS Key Value
#include "CustomCmd.h" // DDS Custom Commands

using namespace std;
using namespace boost::program_options;

int main(int argc, char** argv)
{
    FairMQExample3Processor processor;
    processor.CatchSignals();

    FairMQProgOptions config;

    try
    {
        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        string id = config.GetValue<string>("id");

        LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

        processor.SetTransport(transportFactory);

        processor.SetProperty(FairMQExample3Processor::Id, id);

        // configure data output channel
        FairMQChannel dataInChannel("pull", "connect", "");
        dataInChannel.UpdateRateLogging(0);
        processor.fChannels["data-in"].push_back(dataInChannel);

        // configure data output channel
        FairMQChannel dataOutChannel("push", "connect", "");
        dataOutChannel.UpdateRateLogging(0);
        processor.fChannels["data-out"].push_back(dataOutChannel);

        // Waiting for DDS properties
        dds::key_value::CKeyValue ddsKeyValue;
        // Sampler properties
        dds::key_value::CKeyValue::valuesMap_t samplerValues;
        {
            mutex keyMutex;
            condition_variable keyCondition;

            LOG(INFO) << "Subscribing and waiting for sampler output address.";
            ddsKeyValue.subscribe([&keyCondition](const string& /*_key*/, const string& /*_value*/) { keyCondition.notify_all(); });
            ddsKeyValue.getValues("SamplerAddress", &samplerValues);
            while (samplerValues.empty())
            {
                unique_lock<mutex> lock(keyMutex);
                keyCondition.wait_until(lock, chrono::system_clock::now() + chrono::milliseconds(1000));
                ddsKeyValue.getValues("SamplerAddress", &samplerValues);
            }
        }
        // Sink properties
        dds::key_value::CKeyValue::valuesMap_t sinkValues;
        {
            mutex keyMutex;
            condition_variable keyCondition;

            LOG(INFO) << "Subscribing and waiting for sink input address.";
            ddsKeyValue.subscribe([&keyCondition](const string& /*_key*/, const string& /*_value*/) { keyCondition.notify_all(); });
            ddsKeyValue.getValues("SinkAddress", &sinkValues);
            while (sinkValues.empty())
            {
                unique_lock<mutex> lock(keyMutex);
                keyCondition.wait_until(lock, chrono::system_clock::now() + chrono::milliseconds(1000));
                ddsKeyValue.getValues("SinkAddress", &sinkValues);
            }
        }

        processor.fChannels.at("data-in").at(0).UpdateAddress(samplerValues.begin()->second);
        processor.fChannels.at("data-out").at(0).UpdateAddress(sinkValues.begin()->second);

        processor.ChangeState("INIT_DEVICE");
        processor.WaitForEndOfState("INIT_DEVICE");

        processor.ChangeState("INIT_TASK");
        processor.WaitForEndOfState("INIT_TASK");

        dds::custom_cmd::CCustomCmd ddsCustomCmd;

        // Subscribe on custom commands
        ddsCustomCmd.subscribeCmd([&](const string& command, const string& condition, uint64_t senderId)
        {
            LOG(INFO) << "Received custom command: " << command;
            if (command == "check-state")
            {
                ddsCustomCmd.sendCmd(id + ": " + processor.GetCurrentStateName(), to_string(senderId));
            }
            else
            {
                LOG(WARN) << "Received unknown command: " << command;
                LOG(WARN) << "Origin: " << senderId;
                LOG(WARN) << "Destination: " << condition;
            }
        });

        processor.ChangeState("RUN");
        processor.WaitForEndOfState("RUN");

        processor.ChangeState("RESET_TASK");
        processor.WaitForEndOfState("RESET_TASK");

        processor.ChangeState("RESET_DEVICE");
        processor.WaitForEndOfState("RESET_DEVICE");

        processor.ChangeState("END");
    }
    catch (exception& e)
    {
        LOG(ERROR) << e.what();
        LOG(INFO) << "Command line options are the following: ";
        config.PrintHelp();
        return 1;
    }

    return 0;
}

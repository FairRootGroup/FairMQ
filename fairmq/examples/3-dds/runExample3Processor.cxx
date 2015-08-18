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
#include "FairMQParser.h"
#include "FairMQProgOptions.h"
#include "FairMQExample3Processor.h"
#include "FairMQTools.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

#include "KeyValue.h" // DDS

using namespace boost::program_options;

int main(int argc, char** argv)
{
    FairMQExample3Processor processor;
    processor.CatchSignals();

    FairMQProgOptions config;

    try
    {
        int ddsTaskIndex = 0;

        options_description samplerOptions("Processor options");
        samplerOptions.add_options()
            ("index", value<int>(&ddsTaskIndex)->default_value(0), "Store DDS task index");

        config.AddToCmdLineOptions(samplerOptions);

        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        std::string filename = config.GetValue<std::string>("config-json-file");
        std::string id = config.GetValue<std::string>("id");

        config.UserParser<FairMQParser::JSON>(filename, id);

        processor.fChannels = config.GetFairMQMap();

        LOG(INFO) << "PID: " << getpid();

        // Waiting for DDS properties
        dds::key_value::CKeyValue ddsKeyValue;
        // Sampler properties
        dds::key_value::CKeyValue::valuesMap_t samplerValues;
        {
            mutex keyMutex;
            condition_variable keyCondition;

            LOG(INFO) << "Subscribing and waiting for sampler output address.";
            ddsKeyValue.subscribe([&keyCondition](const string& /*_key*/, const string& /*_value*/) { keyCondition.notify_all(); });
            ddsKeyValue.getValues("SamplerOutputAddress", &samplerValues);
            while (samplerValues.empty())
            {
                unique_lock<mutex> lock(keyMutex);
                keyCondition.wait_until(lock, chrono::system_clock::now() + chrono::milliseconds(1000));
                ddsKeyValue.getValues("SamplerOutputAddress", &samplerValues);
            }
        }
        // Sink properties
        dds::key_value::CKeyValue::valuesMap_t sinkValues;
        {
            mutex keyMutex;
            condition_variable keyCondition;

            LOG(INFO) << "Subscribing and waiting for sink input address.";
            ddsKeyValue.subscribe([&keyCondition](const string& /*_key*/, const string& /*_value*/) { keyCondition.notify_all(); });
            ddsKeyValue.getValues("SinkInputAddress", &sinkValues);
            while (sinkValues.empty())
            {
                unique_lock<mutex> lock(keyMutex);
                keyCondition.wait_until(lock, chrono::system_clock::now() + chrono::milliseconds(1000));
                ddsKeyValue.getValues("SinkInputAddress", &sinkValues);
            }
        }

        processor.fChannels.at("data-in").at(0).UpdateAddress(samplerValues.begin()->second);
        processor.fChannels.at("data-out").at(0).UpdateAddress(sinkValues.begin()->second);

#ifdef NANOMSG
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

        processor.SetTransport(transportFactory);

        processor.SetProperty(FairMQExample3Processor::Id, id);
        processor.SetProperty(FairMQExample3Processor::TaskIndex, ddsTaskIndex);

        processor.ChangeState("INIT_DEVICE");
        processor.WaitForEndOfState("INIT_DEVICE");

        processor.ChangeState("INIT_TASK");
        processor.WaitForEndOfState("INIT_TASK");

        processor.ChangeState("RUN");
        processor.WaitForEndOfState("RUN");

        processor.ChangeState("RESET_TASK");
        processor.WaitForEndOfState("RESET_TASK");

        processor.ChangeState("RESET_DEVICE");
        processor.WaitForEndOfState("RESET_DEVICE");

        processor.ChangeState("END");
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << e.what();
        LOG(INFO) << "Command line options are the following: ";
        config.PrintHelp();
        return 1;
    }

    return 0;
}

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runExample2Sink.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <map>

#include "boost/program_options.hpp"
#include <boost/asio.hpp> // for DDS

#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"
#include "FairMQExample3Sink.h"
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
    FairMQExample3Sink sink;
    sink.CatchSignals();

    FairMQProgOptions config;

    try
    {
        std::string interfaceName; // name of the network interface to use for communication.

        options_description sinkOptions("Sink options");
        sinkOptions.add_options()
            ("network-interface", value<std::string>(&interfaceName)->default_value("eth0"), "Name of the network interface to use (e.g. eth0, ib0, wlan0, en0...)");

        config.AddToCmdLineOptions(sinkOptions);

        if (config.ParseAll(argc, argv))
        {
            return 0;
        }

        std::string filename = config.GetValue<std::string>("config-json-file");
        std::string id = config.GetValue<std::string>("id");

        config.UserParser<FairMQParser::JSON>(filename, id);

        sink.fChannels = config.GetFairMQMap();

        LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
        FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

        sink.SetTransport(transportFactory);

        sink.SetProperty(FairMQExample3Sink::Id, id);

        // Get the IP of the current host and store it for binding.
        map<string,string> IPs;
        FairMQ::tools::getHostIPs(IPs);
        stringstream ss;
        // Check if ib0 (infiniband) interface is available, otherwise try eth0 or wlan0.
        if (IPs.count(interfaceName))
        {
            ss << "tcp://" << IPs[interfaceName] << ":1";
        }
        else
        {
            LOG(INFO) << ss.str();
            LOG(ERROR) << "Could not find provided network interface: \"" << interfaceName << "\"!, exiting.";
            exit(EXIT_FAILURE);
        }
        string initialInputAddress = ss.str();

        // Configure the found host IP for the channel.
        // TCP port will be chosen randomly during the initialization (binding).
        sink.fChannels.at("data-in").at(0).UpdateAddress(initialInputAddress);

        sink.ChangeState("INIT_DEVICE");
        sink.WaitForInitialValidation();

        // Advertise the bound address via DDS property
        LOG(INFO) << "Giving sink input address to DDS.";
        dds::key_value::CKeyValue ddsKeyValue;
        ddsKeyValue.putValue("SinkInputAddress", sink.fChannels.at("data-in").at(0).GetAddress());

        sink.WaitForEndOfState("INIT_DEVICE");

        sink.ChangeState("INIT_TASK");
        sink.WaitForEndOfState("INIT_TASK");

        sink.ChangeState("RUN");
        sink.WaitForEndOfState("RUN");

        sink.ChangeState("RESET_TASK");
        sink.WaitForEndOfState("RESET_TASK");

        sink.ChangeState("RESET_DEVICE");
        sink.WaitForEndOfState("RESET_DEVICE");

        sink.ChangeState("END");
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

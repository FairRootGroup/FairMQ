/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runExampleClient.cxx
 *
 * @since 2013-04-23
 * @author D. Klein, A. Rybalchenko
 */

#include <iostream>

#include "boost/program_options.hpp"

#include "FairMQLogger.h"
#include "FairMQExample5Client.h"

#ifdef NANOMSG
#include "FairMQTransportFactoryNN.h"
#else
#include "FairMQTransportFactoryZMQ.h"
#endif

using namespace std;

typedef struct DeviceOptions
{
    DeviceOptions() :
        text() {}

    string text;
} DeviceOptions_t;

inline bool parse_cmd_line(int _argc, char* _argv[], DeviceOptions* _options)
{
    if (_options == NULL)
        throw runtime_error("Internal error: options' container is empty.");

    namespace bpo = boost::program_options;
    bpo::options_description desc("Options");
    desc.add_options()
        ("text,t", bpo::value<string>()->default_value("something"), "Text to send to server")
        ("help", "Print help messages");

    bpo::variables_map vm;
    bpo::store(bpo::parse_command_line(_argc, _argv, desc), vm);

    if (vm.count("help"))
    {
        LOG(INFO) << "EPN" << endl << desc;
        return false;
    }

    bpo::notify(vm);

    if ( vm.count("text") )
        _options->text = vm["text"].as<string>();

    return true;
}

int main(int argc, char** argv)
{
    FairMQExample5Client client;
    client.CatchSignals();

    DeviceOptions_t options;
    try
    {
        if (!parse_cmd_line(argc, argv, &options))
            return 0;
    }
    catch (exception& e)
    {
        LOG(ERROR) << e.what();
        return 1;
    }

    LOG(INFO) << "PID: " << getpid();

#ifdef NANOMSG
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryNN();
#else
    FairMQTransportFactory* transportFactory = new FairMQTransportFactoryZMQ();
#endif

    client.SetTransport(transportFactory);

    client.SetProperty(FairMQExample5Client::Id, "client");
    client.SetProperty(FairMQExample5Client::Text, options.text);
    client.SetProperty(FairMQExample5Client::NumIoThreads, 1);

    FairMQChannel requestChannel("req", "connect", "tcp://localhost:5005");
    requestChannel.UpdateSndBufSize(10000);
    requestChannel.UpdateRcvBufSize(10000);
    requestChannel.UpdateRateLogging(1);

    client.fChannels["data"].push_back(requestChannel);

    client.ChangeState("INIT_DEVICE");
    client.WaitForEndOfState("INIT_DEVICE");

    client.ChangeState("INIT_TASK");
    client.WaitForEndOfState("INIT_TASK");

    client.ChangeState("RUN");
    client.InteractiveStateLoop();

    return 0;
}

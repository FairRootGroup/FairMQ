/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/*
 * File:   runSamplerRoot.cxx
 * Author: winckler
 */

// FairRoot - FairMQ
#include "FairMQLogger.h"
#include "FairMQProgOptions.h"
#include "FairMQDevice.h"

#include <exception>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

using FairMQMap = unordered_map<string, vector<FairMQChannel>>;

class MyDevice : public FairMQDevice
{
  public:
    MyDevice()
        : fRate(0.5)
    {}

    virtual ~MyDevice()
    {}

    void SetRate(double r)
    {
        fRate = r;
    }

    double GetRate()
    {
        return fRate;
    }

    void Print()
    {
        LOG(INFO) << "[MyDevice] rate = " << fRate;
    }

  private:
    double fRate;
};

void MyCallBack(MyDevice& d, double val)
{
    d.SetRate(val);
    d.Print();
}

int main(int argc, char** argv)
{
    try
    {
        // create option manager object
        FairMQProgOptions config;

        // add key description to cmd line options
        config.GetCmdLineOptions().add_options()
            ("data-rate", po::value<double>()->default_value(0.5), "Data rate");

        // parse command lines, parse json file and init FairMQMap
        config.ParseAll(argc, argv, true);

        // // get FairMQMap
        // auto map1 = config.GetFairMQMap();

        // // update value in variable map, and propagate the update to the FairMQMap
        // config.UpdateValue<string>("chans.data.0.address","tcp://localhost:1234");

        // // get the updated FairMQMap
        // auto map2 = config.GetFairMQMap();

        // // modify one channel value
        // map2.at("data").at(0).UpdateSndBufSize(500);

        // // update the FairMQMap and propagate the change in variable map
        // config.UpdateChannelMap(map2);

        MyDevice device;
        // device.CatchSignals();
        device.SetConfig(config);

        // getting as string and conversion helpers

        // string dataRateStr = config.GetStringValue("data-rate");
        // double dataRate = config.ConvertTo<double>(dataRateStr);
        // LOG(INFO) << "dataRate: " << dataRate;

        LOG(INFO) << "Subscribing: <string>(chans.data.0.address)";
        config.Subscribe<string>("test", [&device](const string& key, string value)
        {
            if (key == "chans.data.0.address")
            {
                LOG(INFO) << "[callback] Updating device parameter " << key << " = " << value;
                device.fChannels.at("data").at(0).UpdateAddress(value);
            }
        });

        LOG(INFO) << "Subscribing: <int>(chans.data.0.rcvBufSize)";
        config.Subscribe<int>("test", [&device](const string& key, int value)
        {
            if(key == "chans.data.0.rcvBufSize")
            {
                LOG(INFO) << "[callback] Updating device parameter " << key << " = " << value;
                device.fChannels.at("data").at(0).UpdateRcvBufSize(value);
            }
        });

        LOG(INFO) << "Subscribing: <double>(data-rate)";
        config.Subscribe<double>("test", [&device](const string& key, double value)
        {
            if (key == "data-rate")
            {
                LOG(INFO) << "[callback] Updating device parameter " << key << " = " << value;
                device.SetRate(value);
            }
        });

        LOG(INFO) << "Starting value updates...\n";

        config.UpdateValue<string>("chans.data.0.address", "tcp://localhost:4321");
        LOG(INFO) << "config: " << config.GetValue<string>("chans.data.0.address");
        LOG(INFO) << "device: " << device.fChannels.at("data").at(0).GetAddress() << endl;

        config.UpdateValue<int>("chans.data.0.rcvBufSize", 100);
        LOG(INFO) << "config: " << config.GetValue<int>("chans.data.0.rcvBufSize");
        LOG(INFO) << "device: " << device.fChannels.at("data").at(0).GetRcvBufSize() << endl;

        config.UpdateValue<double>("data-rate", 0.9);
        LOG(INFO) << "config: " << config.GetValue<double>("data-rate");
        LOG(INFO) << "device: " << device.GetRate() << endl;
        // device.Print();

        LOG(INFO) << "nase: " << config.GetValue<double>("nase");

        config.Unsubscribe<string>("test");
        config.Unsubscribe<int>("test");
        config.Unsubscribe<double>("test");
        // advanced commands

        // LOG(INFO) << "-------------------- start custom 1";

        // config.Connect<EventId::Custom, MyDevice&, double>("myNewKey", [](MyDevice& d, double val)
        // {
        //     d.SetRate(val);
        //     d.Print();
        // });

        // config.Emit<EventId::Custom, MyDevice&, double>("myNewKey", device, 0.123);

        // LOG(INFO) << "-------------------- start custom 2 with function";
        // config.Connect<EventId::Custom, MyDevice&, double>("function example", &MyCallBack);

        // config.Emit<EventId::Custom, MyDevice&, double>("function example", device, 6.66);
    }
    catch (exception& e)
    {
        LOG(ERROR)  << "Unhandled Exception reached the top of main: " 
                    << e.what() << ", application will now exit";
        return 1;
    }
    return 0;
}

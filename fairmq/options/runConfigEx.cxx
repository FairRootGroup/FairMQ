/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
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
        LOG(info) << "[MyDevice] rate = " << fRate;
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
        FairMQProgOptions config;

        config.GetCmdLineOptions().add_options()
            ("data-rate", boost::program_options::value<double>()->default_value(0.5), "Data rate");

        config.ParseAll(argc, argv, true);

        // // get FairMQMap
        // auto map1 = config.GetFairMQMap();

        // // update value in variable map, and propagate the update to the FairMQMap
        // config.SetValue<string>("chans.data.0.address","tcp://localhost:1234");

        // // get the updated FairMQMap
        // auto map2 = config.GetFairMQMap();

        // // modify one channel value
        // map2.at("data").at(0).UpdateSndBufSize(500);

        // // update the FairMQMap and propagate the change in variable map
        // config.UpdateChannelMap(map2);

        MyDevice device;
        device.SetConfig(config);

        // getting as string and conversion helpers

        // string dataRateStr = config.GetStringValue("data-rate");
        // double dataRate = config.ConvertTo<double>(dataRateStr);
        // LOG(info) << "dataRate: " << dataRate;

        LOG(info) << "Subscribing: <string>(chans.data.0.address)";
        config.Subscribe<string>("test", [&device](const string& key, string value)
        {
            if (key == "chans.data.0.address")
            {
                LOG(info) << "[callback] Updating device parameter " << key << " = " << value;
                device.fChannels.at("data").at(0).UpdateAddress(value);
            }
        });

        LOG(info) << "Subscribing: <int>(chans.data.0.rcvBufSize)";
        config.Subscribe<int>("test", [&device](const string& key, int value)
        {
            if(key == "chans.data.0.rcvBufSize")
            {
                LOG(info) << "[callback] Updating device parameter " << key << " = " << value;
                device.fChannels.at("data").at(0).UpdateRcvBufSize(value);
            }
        });

        LOG(info) << "Subscribing: <double>(data-rate)";
        config.Subscribe<double>("test", [&device](const string& key, double value)
        {
            if (key == "data-rate")
            {
                LOG(info) << "[callback] Updating device parameter " << key << " = " << value;
                device.SetRate(value);
            }
        });

        LOG(info) << "Starting value updates...\n";

        config.SetValue<string>("chans.data.0.address", "tcp://localhost:4321");
        LOG(info) << "config: " << config.GetValue<string>("chans.data.0.address");
        LOG(info) << "device: " << device.fChannels.at("data").at(0).GetAddress() << endl;

        config.SetValue<int>("chans.data.0.rcvBufSize", 100);
        LOG(info) << "config: " << config.GetValue<int>("chans.data.0.rcvBufSize");
        LOG(info) << "device: " << device.fChannels.at("data").at(0).GetRcvBufSize() << endl;

        config.SetValue<double>("data-rate", 0.9);
        LOG(info) << "config: " << config.GetValue<double>("data-rate");
        LOG(info) << "device: " << device.GetRate() << endl;
        device.Print();

        LOG(info) << "nase: " << config.GetValue<double>("nase");

        config.Unsubscribe<string>("test");
        config.Unsubscribe<int>("test");
        config.Unsubscribe<double>("test");

        device.ChangeState("END");
    }
    catch (exception& e)
    {
        LOG(error) << "Unhandled Exception reached the top of main: " << e.what() << ", application will now exit";
        return 1;
    }
    return 0;
}

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

typedef std::unordered_map<std::string, std::vector<FairMQChannel>> FairMQMap;

class MyDevice : public FairMQDevice 
{
  public:
    MyDevice() : rate(0.5) {}
    virtual ~MyDevice() {}
    void SetRate(double r){rate=r;}
    void Print(){LOG(INFO)<<"[MyDevice] rate = "<<rate;}
  private:
    double rate;
};

void MyCallBack(MyDevice& d, double val)
            {
                d.SetRate(val);
                d.Print();
            }

void PrintMQParam(const FairMQMap& channels, const FairMQProgOptions& config)
{

    for(const auto& p : channels)
    {
        int index = 0;
        for(const auto& channel : p.second)
        {
            std::string typeKey = p.first + "." + std::to_string(index) + ".type";
            std::string methodKey = p.first + "." + std::to_string(index) + ".method";
            std::string addressKey = p.first + "." + std::to_string(index) + ".address";
            std::string propertyKey = p.first + "." + std::to_string(index) + ".property";
            std::string sndBufSizeKey = p.first + "." + std::to_string(index) + ".sndBufSize";
            std::string rcvBufSizeKey = p.first + "." + std::to_string(index) + ".rcvBufSize";
            std::string rateLoggingKey = p.first + "." + std::to_string(index) + ".rateLogging";
            
            LOG(DEBUG) << "Channel name = "<<p.first;
            LOG(DEBUG) << "key = " << typeKey <<"\t value = " << config.GetValue<std::string>(typeKey);
            LOG(DEBUG) << "key = " << methodKey <<"\t value = " << config.GetValue<std::string>(methodKey);
            LOG(DEBUG) << "key = " << addressKey <<"\t value = " << config.GetValue<std::string>(addressKey);
            LOG(DEBUG) << "key = " << propertyKey <<"\t value = " << config.GetValue<std::string>(propertyKey);
            LOG(DEBUG) << "key = " << sndBufSizeKey << "\t value = " << config.GetValue<int>(sndBufSizeKey);
            LOG(DEBUG) << "key = " << rcvBufSizeKey <<"\t value = " << config.GetValue<int>(rcvBufSizeKey);
            LOG(DEBUG) << "key = " << rateLoggingKey <<"\t value = " << config.GetValue<int>(rateLoggingKey);
        }
    }
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
        config.ParseAll(argc, argv);

        // get FairMQMap
        auto map1 = config.GetFairMQMap();
        
        // form keys from map1 and print the value stored in variable map
        PrintMQParam(map1,config);

        // update value in variable map, and propagate the update to the FairMQMap
        config.UpdateValue("data.0.address","tcp://localhost:1234");
        
        // get the updated FairMQMap
        auto map2 = config.GetFairMQMap();

        // modify one channel value
        map2.at("data").at(0).UpdateSndBufSize(500);

        // update the FairMQMap and propagate the change in variable map
        config.UpdateChannelMap(map2);

        // print values stored in variable map 
        PrintMQParam(map2,config);

        MyDevice device;
        device.CatchSignals();
        device.SetConfig(config);


        std::string blah = config.GetStringValue("data-rate");
        double blah2 = config.ConvertTo<double>(blah);
        LOG(INFO)<<"blah2 "<<blah2;


        LOG(INFO)<<"---- Connect 1";
        config.Subscribe<std::string >("data.0.address",[&device](const std::string& key, const std::string& value) 
            {
                LOG(INFO) << "[Lambda] Update parameter (0) " << key << " = " << value; 
                device.fChannels.at("data").at(0).UpdateAddress(value);
            });


        std::string key1("data.0.address");
        std::string value1("tcp://localhost:4321");
        config.UpdateValue(key1,value1);

        LOG(INFO)<<"device.fChannels.GetAddress = "<<device.fChannels.at("data").at(0).GetAddress();
        LOG(INFO)<<"config.GetValue = "<<config.GetValue<std::string>(key1);




        LOG(INFO)<<"---- Connect 2";
        config.Subscribe<std::string>("data.0.method",[&device](const std::string& key, const std::string& value) 
            {
                //value="abcd";
                LOG(INFO) << "[Lambda] Update parameter " << key << " = " << value; 
                device.fChannels.at("data").at(0).UpdateMethod(value);
            });

        LOG(INFO)<<"---- Connect 3";
        config.Subscribe<int>("data.0.rcvBufSize",[&device](const std::string& key, int value) 
            {
                LOG(INFO) << "[Lambda] Update parameter " << key << " = " << value; 
                device.fChannels.at("data").at(0).UpdateRcvBufSize(value);
            });

        LOG(INFO)<<"---- Connect 4";
        config.Subscribe<double>("data-rate",[&device](const std::string& key, double value) 
            {
                LOG(INFO) << "[Lambda] Update parameter " << key << " = " << value; 
                device.SetRate(value);
            });


        std::string key2("data.0.rcvBufSize");
        int value2(100);

        std::string key3("data.0.method");
        std::string value3("bind");
        


        LOG(INFO)<<"-------------------- start update";

        //config.EmitUpdate(key,value);
        config.UpdateValue(key1,value1);

        LOG(INFO)<<"device.fChannels.GetAddress = "<<device.fChannels.at("data").at(0).GetAddress();
        LOG(INFO)<<"config.GetValue = "<<config.GetValue<std::string>(key1);

        config.UpdateValue(key2,value2);

        LOG(INFO)<<"device.fChannels.GetRcvBufSize = "<<device.fChannels.at("data").at(0).GetRcvBufSize();
        LOG(INFO)<<"config.GetValue = "<<config.GetValue<int>(key2);

        config.UpdateValue(key3,value3);

        LOG(INFO)<<"device.fChannels.Method = "<<device.fChannels.at("data").at(0).GetMethod();
        LOG(INFO)<<"config.GetValue = "<<config.GetValue<std::string>(key3);

        device.Print();
        double rate=0.9;
        config.UpdateValue<double>("data-rate",rate);
        LOG(INFO)<<"config.GetValue = "<<config.GetValue<double>("data-rate");
        device.Print();
        LOG(INFO)<<" double rate = " <<rate;


        // advanced commands

        LOG(INFO)<<"-------------------- start custom 1";

        config.Connect<EventId::Custom, MyDevice&, double>("myNewKey",[](MyDevice& d, double val)
            {
                d.SetRate(val);
                d.Print();
            });

        double value4=0.123;
        config.Emit<EventId::Custom, MyDevice&, double>("myNewKey",device,value4);


        LOG(INFO)<<"-------------------- start custom 2 with function";
        config.Connect<EventId::Custom, MyDevice&, double>("function example",&MyCallBack);

        value4=6.66;
        config.Emit<EventId::Custom, MyDevice&, double>("function example",device,value4);




    }
    catch (std::exception& e)
    {
        LOG(ERROR)  << "Unhandled Exception reached the top of main: " 
                    << e.what() << ", application will now exit";
        return 1;
    }
    return 0;
}



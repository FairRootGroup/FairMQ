/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/DeviceRunner.h>
#include <boost/program_options.hpp>
#include <memory>
#include <string>

template <typename R>
class GenericFairMQDevice : public FairMQDevice
{
  public:
    GenericFairMQDevice(R func) : r(func) {}

  protected:
    virtual bool ConditionalRun() { return r(*static_cast<FairMQDevice*>(this)); }

  private:
    R r;
};

template <typename R>
FairMQDevice* makeDeviceWithConditionalRun(R r)
{
    return new GenericFairMQDevice<R>(r);
}

using FairMQDevicePtr = FairMQDevice*;

// to be implemented by the user to return a child class of FairMQDevice
FairMQDevicePtr getDevice(const FairMQProgOptions& config);

// to be implemented by the user to add custom command line options (or just with empty body)
void addCustomOptions(boost::program_options::options_description&);

int main(int argc, char* argv[])
{
    using namespace fair::mq;
    using namespace fair::mq::hooks;

    fair::mq::DeviceRunner runner{argc, argv};

    // runner.AddHook<LoadPlugins>([](DeviceRunner& r){
    //     // for example:
    //     r.fPluginManager->SetSearchPaths({"/lib", "/lib/plugins"});
    //     r.fPluginManager->LoadPlugin("asdf");
    // });

    runner.AddHook<SetCustomCmdLineOptions>([](DeviceRunner& r){
        boost::program_options::options_description customOptions("Custom options");
        addCustomOptions(customOptions);
        r.fConfig.AddToCmdLineOptions(customOptions);
    });

    // runner.AddHook<ModifyRawCmdLineArgs>([](DeviceRunner& r){
    //     // for example:
    //     r.fRawCmdLineArgs.push_back("--blubb");
    // });

    runner.AddHook<InstantiateDevice>([](DeviceRunner& r){
        r.fDevice = std::shared_ptr<FairMQDevice>{getDevice(r.fConfig)};
    });

    return runner.RunWithExceptionHandlers();

    // Run without builtin catch all exception handler, just:
    // return runner.Run();
}

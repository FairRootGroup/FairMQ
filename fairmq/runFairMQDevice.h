/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <boost/program_options.hpp>

#include "FairMQLogger.h"
#include "FairMQProgOptions.h"
#include "FairMQDevice.h"
#include "runSimpleMQStateMachine.h"

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

// to be implemented by the user to return a child class of FairMQDevice
FairMQDevice* getDevice(const FairMQProgOptions& config);

// to be implemented by the user to add custom command line options (or just with empty body)
void addCustomOptions(boost::program_options::options_description&);

int main(int argc, char** argv)
{
    try
    {
        boost::program_options::options_description customOptions("Custom options");
        addCustomOptions(customOptions);

        FairMQProgOptions config;
        config.AddToCmdLineOptions(customOptions);
        config.ParseAll(argc, argv);

        std::unique_ptr<FairMQDevice> device(getDevice(config));
        int result = runStateMachine(*device, config);

        if (result > 0)
        {
            return 1;
        }
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Unhandled exception reached the top of main: " << e.what() << ", application will now exit";
        return 1;
    }

    return 0;
}

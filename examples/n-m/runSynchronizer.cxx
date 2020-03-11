/********************************************************************************
 *    Copyright (C) 2020 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <FairMQDevice.h>
#include <runFairMQDevice.h>

#include <string>
#include <cstdint>

using namespace std;
namespace bpo = boost::program_options;

class Synchronizer : public FairMQDevice
{
  public:
    Synchronizer()
        : fTimeframeId(0)
    {}
    ~Synchronizer() = default;

  protected:
    bool ConditionalRun() override
    {
        FairMQMessagePtr msg(NewSimpleMessage(fTimeframeId));

        if (Send(msg, "sync") > 0) {
            if (++fTimeframeId == UINT16_MAX - 1) {
                fTimeframeId = 0;
            }
        } else {
            return false;
        }

        return true;
    }

    uint16_t fTimeframeId;
};

void addCustomOptions(bpo::options_description& /* options */) {}
FairMQDevice* getDevice(const FairMQProgOptions& /* config */) { return new Synchronizer(); }

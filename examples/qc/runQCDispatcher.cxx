/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "runFairMQDevice.h"
#include "FairMQDevice.h"

class QCDispatcher : public FairMQDevice
{
  public:
    QCDispatcher()
        : fDoQC(false)
    {
        OnData("data1", &QCDispatcher::HandleData);
    }

    void InitTask() override
    {
        GetConfig()->Subscribe<std::string>("qcdevice", [&](const std::string& key, std::string value) {
            if (key == "qc") {
                if (value == "active") {
                    fDoQC.store(true);
                } else if (value == "inactive") {
                    fDoQC.store(false);
                }
            }
        });
    }

  protected:
    bool HandleData(FairMQMessagePtr& msg, int)
    {
        if (fDoQC.load() == true) {
            FairMQMessagePtr msgCopy(NewMessage());
            msgCopy->Copy(*msg);
            if (Send(msg, "qc") < 0) {
                return false;
            }
        }

        if (Send(msg, "data2") < 0) {
            return false;
        }

        return true;
    }

    void ResetTask() override { GetConfig()->Unsubscribe<std::string>("qcdevice"); }

  private:
    std::atomic<bool> fDoQC;
};

void addCustomOptions(boost::program_options::options_description& /*options*/) {}
FairMQDevicePtr getDevice(const fair::mq::ProgOptions& /*config*/) { return new QCDispatcher(); }

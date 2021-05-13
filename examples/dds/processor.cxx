/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <string>

namespace bpo = boost::program_options;

class Processor : public FairMQDevice
{
  public:
    Processor()
    {
        OnData("data1", &Processor::HandleData);
    }

  protected:
    bool HandleData(FairMQMessagePtr& msg, int)
    {
        LOG(info) << "Received data, processing...";

        // Modify the received string
        std::string* text = new std::string(static_cast<char*>(msg->GetData()), msg->GetSize());
        *text += " (modified by " + fId + ")";

        // create message object with a pointer to the data buffer,
        // its size,
        // custom deletion function (called when transfer is done),
        // and pointer to the object managing the data buffer
        FairMQMessagePtr msg2(NewMessage(const_cast<char*>(text->c_str()),
                                        text->length(),
                                        [](void* /*data*/, void* object) { delete static_cast<std::string*>(object); },
                                        text));

        // Send out the output message
        if (Send(msg2, "data2") < 0) {
            return false;
        }

        return true;
    }
};

void addCustomOptions(bpo::options_description& /*options*/)
{
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Processor>();
}

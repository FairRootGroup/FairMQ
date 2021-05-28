/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Header.h"

#include <fairmq/Device.h>
#include <fairmq/runDevice.h>

#include <chrono>
#include <cstdint>
#include <thread> // this_thread::sleep_for

namespace bpo = boost::program_options;

struct Sampler : fair::mq::Device
{
    void InitTask() override
    {
        fMaxIterations = fConfig->GetProperty<uint64_t>("max-iterations");
    }

    bool ConditionalRun() override
    {
        example_multipart::Header header;
        header.stopFlag = 0;

        // Set stopFlag to 1 for last message.
        if (fMaxIterations > 0 && fNumIterations == fMaxIterations - 1) {
            header.stopFlag = 1;
        }
        LOG(info) << "Sending header with stopFlag: " << header.stopFlag;

        FairMQParts parts;

        // NewSimpleMessage creates a copy of the data and takes care of its destruction (after the transfer takes place).
        // Should only be used for small data because of the cost of an additional copy
        parts.AddPart(NewSimpleMessage(header));
        parts.AddPart(NewMessage(1000));

        // create more data parts, testing the FairMQParts in-place constructor
        FairMQParts auxData{ NewMessage(500), NewMessage(600), NewMessage(700) };
        assert(auxData.Size() == 3);
        parts.AddPart(std::move(auxData));
        assert(auxData.Size() == 0);
        assert(parts.Size() == 5);

        parts.AddPart(NewMessage());

        assert(parts.Size() == 6);

        parts.AddPart(NewMessage(100));

        assert(parts.Size() == 7);

        LOG(info) << "Sending body of size: " << parts.At(1)->GetSize();

        Send(parts, "data");

        // Go out of the sending loop if the stopFlag was sent.
        if (fMaxIterations > 0 && ++fNumIterations >= fMaxIterations) {
            LOG(info) << "Configured maximum number of iterations reached. Leaving RUNNING state.";
            return false;
        }

        // Wait a second to keep the output readable.
        std::this_thread::sleep_for(std::chrono::seconds(1));

        return true;
    }

  private:
    uint64_t fMaxIterations = 5;
    uint64_t fNumIterations = 0;
};

void addCustomOptions(bpo::options_description& options)
{
    options.add_options()
        ("max-iterations", bpo::value<uint64_t>()->default_value(5), "Maximum number of iterations of Run/ConditionalRun/OnData (0 - infinite)");
}

std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& /*config*/)
{
    return std::make_unique<Sampler>();
}

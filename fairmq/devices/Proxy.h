/********************************************************************************
 * Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PROXY_H
#define FAIR_MQ_PROXY_H

#include <fairmq/Device.h>

#include <string>

namespace fair::mq
{

class Proxy : public Device
{
  public:
    Proxy() = default;

  protected:
    bool fMultipart = true;
    std::string fInChannelName;
    std::string fOutChannelName;

    void InitTask() override
    {
        fMultipart = fConfig->GetProperty<bool>("multipart");
        fInChannelName = fConfig->GetProperty<std::string>("in-channel");
        fOutChannelName = fConfig->GetProperty<std::string>("out-channel");
    }

    void Run() override
    {
        if (fMultipart) {
            while (!NewStatePending()) {
                FairMQParts payload;
                if (Receive(payload, fInChannelName) >= 0) {
                    if (Send(payload, fOutChannelName) < 0) {
                        LOG(debug) << "Transfer interrupted";
                        break;
                    }
                } else {
                    LOG(debug) << "Transfer interrupted";
                    break;
                }
            }
        } else {
            while (!NewStatePending()) {
                FairMQMessagePtr payload(fTransportFactory->CreateMessage());
                if (Receive(payload, fInChannelName) >= 0) {
                    if (Send(payload, fOutChannelName) < 0) {
                        LOG(debug) << "Transfer interrupted";
                        break;
                    }
                } else {
                    LOG(debug) << "Transfer interrupted";
                    break;
                }
            }
        }
    }
};

} // namespace fair::mq

#endif /* FAIR_MQ_PROXY_H */

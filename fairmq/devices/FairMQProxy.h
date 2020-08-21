/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQProxy.h
 *
 * @since 2013-10-02
 * @author A. Rybalchenko
 */

#ifndef FAIRMQPROXY_H_
#define FAIRMQPROXY_H_

#include "FairMQDevice.h"

#include <string>

class FairMQProxy : public FairMQDevice
{
  public:
    FairMQProxy()
        : fMultipart(true)
        , fInChannelName()
        , fOutChannelName()
    {}
    ~FairMQProxy() {}

  protected:
    bool fMultipart;
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

#endif /* FAIRMQPROXY_H_ */

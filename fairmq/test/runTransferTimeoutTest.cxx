/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * runTransferTimeoutTester.cxx
 *
 * @since 2015-09-05
 * @author A. Rybalchenko
 */

#include "FairMQLogger.h"
#include "FairMQDevice.h"

class TransferTimeoutTester : public FairMQDevice
{
  public:
    TransferTimeoutTester() {}
    virtual ~TransferTimeoutTester() {}

  protected:
    virtual void Run()
    {
//        bool setSndOK = false;
//        bool setRcvOK = false;
        bool getSndOK = false;
        bool getRcvOK = false;
        bool sendCanceling = false;
        bool receiveCanceling = false;

        fChannels.at("data-out").at(0).SetSendTimeout(1000);
        fChannels.at("data-in").at(0).SetReceiveTimeout(1000);

        if (fChannels.at("data-out").at(0).GetSendTimeout() == 1000)
        {
            getSndOK = true;
            LOG(INFO) << "get send timeout OK: " << fChannels.at("data-out").at(0).GetSendTimeout();
        }
        else
        {
            LOG(ERROR) << "get send timeout failed";
        }

        if (fChannels.at("data-in").at(0).GetReceiveTimeout() == 1000)
        {
            getRcvOK = true;
            LOG(INFO) << "get receive timeout OK: " << fChannels.at("data-in").at(0).GetReceiveTimeout();
        }
        else
        {
            LOG(ERROR) << "get receive timeout failed";
        }

        if (getSndOK && getRcvOK)
        {
            std::unique_ptr<FairMQMessage> msg1(NewMessage());
            std::unique_ptr<FairMQMessage> msg2(NewMessage());

            if (Send(msg1, "data-out") == -2)
            {
                LOG(INFO) << "send canceled";
                sendCanceling = true;
            }
            else
            {
                LOG(ERROR) << "send did not cancel";
            }

            if (Receive(msg2, "data-in") == -2)
            {
                LOG(INFO) << "receive canceled";
                receiveCanceling = true;
            }
            else
            {
                LOG(ERROR) << "receive did not cancel";
            }

            if (sendCanceling && receiveCanceling)
            {
                LOG(INFO) << "Transfer timeout test successfull";
            }
        }
    }
};

int main(int argc, char** argv)
{
    TransferTimeoutTester timeoutTester;
    timeoutTester.CatchSignals();
    if (argc == 2)
    {
        timeoutTester.SetTransport(argv[1]);
    }
    else
    {
        timeoutTester.SetTransport("zeromq");
    }

    reinit_logger(false);

    timeoutTester.SetProperty(TransferTimeoutTester::Id, "timeoutTester");

    FairMQChannel dataOutChannel;
    dataOutChannel.UpdateType("push");
    dataOutChannel.UpdateMethod("bind");
    dataOutChannel.UpdateAddress("tcp://127.0.0.1:5559");
    if (argc == 2)
    {
        dataOutChannel.UpdateAddress("tcp://127.0.0.1:5759");
    }
    dataOutChannel.UpdateSndBufSize(1000);
    dataOutChannel.UpdateRcvBufSize(1000);
    dataOutChannel.UpdateRateLogging(0);
    timeoutTester.fChannels["data-out"].push_back(dataOutChannel);

    FairMQChannel dataInChannel;
    dataInChannel.UpdateType("pull");
    dataInChannel.UpdateMethod("bind");
    dataInChannel.UpdateAddress("tcp://127.0.0.1:5560");
    if (argc == 2)
    {
        dataInChannel.UpdateAddress("tcp://127.0.0.1:5760");
    }
    dataInChannel.UpdateSndBufSize(1000);
    dataInChannel.UpdateRcvBufSize(1000);
    dataInChannel.UpdateRateLogging(0);
    timeoutTester.fChannels["data-in"].push_back(dataInChannel);

    timeoutTester.ChangeState(TransferTimeoutTester::INIT_DEVICE);
    timeoutTester.WaitForEndOfState(TransferTimeoutTester::INIT_DEVICE);

    timeoutTester.ChangeState(TransferTimeoutTester::INIT_TASK);
    timeoutTester.WaitForEndOfState(TransferTimeoutTester::INIT_TASK);

    timeoutTester.ChangeState(TransferTimeoutTester::RUN);
    timeoutTester.WaitForEndOfState(TransferTimeoutTester::RUN);

    timeoutTester.ChangeState(TransferTimeoutTester::RESET_TASK);
    timeoutTester.WaitForEndOfState(TransferTimeoutTester::RESET_TASK);

    timeoutTester.ChangeState(TransferTimeoutTester::RESET_DEVICE);
    timeoutTester.WaitForEndOfState(TransferTimeoutTester::RESET_DEVICE);

    timeoutTester.ChangeState(TransferTimeoutTester::END);

    return 0;
}

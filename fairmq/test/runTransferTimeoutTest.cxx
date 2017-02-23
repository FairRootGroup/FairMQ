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

#include <chrono>
#include <thread>

class TransferTimeoutTester : public FairMQDevice
{
  public:
    TransferTimeoutTester() {}
    virtual ~TransferTimeoutTester() {}

  protected:
    virtual void Run()
    {
        bool sendCanceling = false;
        bool receiveCanceling = false;

        FairMQMessagePtr msg1(NewMessage());
        FairMQMessagePtr msg2(NewMessage());

        if (Send(msg1, "data-out", 0, 1000) == -2)
        {
            LOG(INFO) << "send canceled";
            sendCanceling = true;
        }
        else
        {
            LOG(ERROR) << "send did not cancel";
        }

        if (Receive(msg2, "data-in", 0, 1000) == -2)
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
};

int main(int argc, char** argv)
{
    TransferTimeoutTester timeoutTester;
    timeoutTester.CatchSignals();

    std::string transport;
    if ( (argc != 2) || (argv[1] == NULL) )
    {
        LOG(ERROR) << "Transport for the test not specified!";
        return 1;
    }


    if ( strncmp(argv[1],"zeromq",6) == 0 )
    {
        transport = "zeromq";
        timeoutTester.SetTransport(transport);
    }
    else if ( strncmp(argv[1],"nanomsg",7) == 0 )
    {
        transport = "nanomsg";
        timeoutTester.SetTransport(transport);
    }
    else
    {
        LOG(ERROR) << "Incorrect transport requested! Expected 'zeromq' or 'nanomsg', found: " << argv[1];
        return 1;
    }

    reinit_logger(false);

    timeoutTester.SetProperty(TransferTimeoutTester::Id, "timeoutTester");

    FairMQChannel dataOutChannel;
    dataOutChannel.UpdateType("push");
    dataOutChannel.UpdateMethod("bind");
    dataOutChannel.UpdateAddress("tcp://127.0.0.1:5559");
    if (transport == "nanomsg")
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
    if (transport == "nanomsg")
    {
        dataInChannel.UpdateAddress("tcp://127.0.0.1:5760");
    }
    dataInChannel.UpdateSndBufSize(1000);
    dataInChannel.UpdateRcvBufSize(1000);
    dataInChannel.UpdateRateLogging(0);
    timeoutTester.fChannels["data-in"].push_back(dataInChannel);

    timeoutTester.ChangeState("INIT_DEVICE");
    timeoutTester.WaitForEndOfState("INIT_DEVICE");

    timeoutTester.ChangeState("INIT_TASK");
    timeoutTester.WaitForEndOfState("INIT_TASK");

    timeoutTester.ChangeState("RUN");
    timeoutTester.WaitForEndOfState("RUN");

    // nanomsg does not implement the LINGER option. Give the sockets some time before their queues are terminated
    if (transport == "nanomsg")
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    timeoutTester.ChangeState("RESET_TASK");
    timeoutTester.WaitForEndOfState("RESET_TASK");

    timeoutTester.ChangeState("RESET_DEVICE");
    timeoutTester.WaitForEndOfState("RESET_DEVICE");

    timeoutTester.ChangeState("END");

    return 0;
}

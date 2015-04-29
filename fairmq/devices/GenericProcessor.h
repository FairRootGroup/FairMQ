/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/* 
 * File:   GenericProcessor.h
 * Author: winckler
 *
 * Created on December 1, 2014, 10:22 PM
 */

#ifndef GENERICPROCESSOR_H
#define GENERICPROCESSOR_H

#include "FairMQDevice.h"

/*********************************************************************
 * -------------- NOTES -----------------------
 * All policies must have a default constructor
 * Function to define in (parent) policy classes :
 * 
 *  -------- INPUT POLICY --------
 *                InputPolicy::InitContainer(...)
 * CONTAINER_TYPE InputPolicy::DeSerializeMsg(FairMQMessage* msg)
 *                InputPolicy::InitContainer(...)  // if GenericProcessor::InitInputContainer(...) is used
 * 
 * 
 *  -------- OUTPUT POLICY --------
 *                OutputPolicy::SerializeMsg(CONTAINER_TYPE)
 *                OutputPolicy::SetMessage(FairMQMessage* msg)
 *                OutputPolicy::InitContainer(...)  // if GenericProcessor::InitOutputContainer(...) is used
 * 
 *  -------- TASK POLICY --------
 * CONTAINER_TYPE TaskPolicy::GetOutputData()
 *                TaskPolicy::ExecuteTask(CONTAINER_TYPE container)
 *                TaskPolicy::InitTask(...)  // if GenericProcessor::InitTask(...) is used
 *                
 **********************************************************************/

template <typename InputPolicy, typename OutputPolicy, typename TaskPolicy>
class GenericProcessor : public FairMQDevice, public InputPolicy, public OutputPolicy, public TaskPolicy
{
  public:
    GenericProcessor()
        : InputPolicy()
        , OutputPolicy()
        , TaskPolicy()
    {}

    virtual ~GenericProcessor()
    {}

    // the four following methods ensure 
    // that the correct policy method is called

    void SetTransport(FairMQTransportFactory* transport)
    {
        FairMQDevice::SetTransport(transport);
    }

    template <typename... Args>
    void InitTask(Args... args)
    {
        TaskPolicy::InitTask(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void InitInputContainer(Args... args)
    {
        InputPolicy::InitContainer(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void InitOutputContainer(Args... args)
    {
        OutputPolicy::InitContainer(std::forward<Args>(args)...);
    }

    /*
     * 
    // ***********************  TODO: implement multipart features
    void SendPart()
    {
        fChannels["data-out"].at(0).Send(OutputPolicy::SerializeMsg(TaskPolicy::GetData()), "snd-more");
        OutputPolicy::CloseMessage(); 
    }

    // void SendPart();
    // bool ReceivePart();
    bool ReceivePart()
    {
        int64_t more = 0;
        size_t more_size = sizeof(more);
        fChannels["data-in"].at(0).fSocket->GetOption("rcv-more", &more, &more_size);
        if (more)
        {
            InputPolicy::CloseMessage(); 
            // fProcessorTask->GetPayload()->CloseMessage();
            fProcessorTask->SetPayload(fTransportFactory->CreateMessage());
            return fChannels["data-in"].at(0).Receive(fProcessorTask->GetPayload());
        }
        else
        {
            return false;
        }
    }
    */

  protected:
    virtual void InitTask()
    {
        // TODO: implement multipart features
        // fProcessorTask->InitTask();
        // fProcessorTask->SetSendPart(boost::bind(&FairMQProcessor::SendPart, this));
        // fProcessorTask->SetReceivePart(boost::bind(&FairMQProcessor::ReceivePart, this));
    }

    virtual void Run()
    {
        int receivedMsgs = 0;
        int sentMsgs = 0;

        while (GetCurrentState() == RUNNING)
        {
            FairMQMessage* msg = fTransportFactory->CreateMessage();
            
            receivedMsgs++;

            // InputPolicy::DeSerializeMsg(msg) --> deserialize data of msg and fill output container
            // TaskPolicy::ExecuteTask( ... )   --> process output container
            TaskPolicy::ExecuteTask(InputPolicy::DeSerializeMsg(msg));

            // OutputPolicy::fMessage point to msg
            OutputPolicy::SetMessage(msg);

            if (fChannels["data-in"].at(0).Receive(msg) > 0)
            {
                // TaskPolicy::GetOutputData() --> Get processed output container
                // OutputPolicy::message(...)  --> Serialize output container and fill fMessage
                fChannels["data-out"].at(0).Send(OutputPolicy::SerializeMsg(TaskPolicy::GetOutputData()));
                sentMsgs++;
            }

            if (msg)
            {
                msg->CloseMessage();
            }
        }

        MQLOG(INFO) << "Received " << receivedMsgs << " and sent " << sentMsgs << " messages!";
    }

};

#endif	/* GENERICPROCESSOR_H */


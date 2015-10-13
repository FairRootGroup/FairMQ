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
 *                deserialization_type::InitContainer(...)
 * CONTAINER_TYPE deserialization_type::DeSerializeMsg(FairMQMessage* msg)
 *                deserialization_type::InitContainer(...)  // if GenericProcessor::InitInputContainer(...) is used
 * 
 * 
 *  -------- OUTPUT POLICY --------
 *                serialization_type::SerializeMsg(CONTAINER_TYPE)
 *                serialization_type::SetMessage(FairMQMessage* msg)
 *                serialization_type::InitContainer(...)  // if GenericProcessor::InitOutputContainer(...) is used
 * 
 *  -------- TASK POLICY --------
 * CONTAINER_TYPE proc_task_type::GetOutputData()
 *                proc_task_type::ExecuteTask(CONTAINER_TYPE container)
 *                proc_task_type::InitTask(...)  // if GenericProcessor::InitTask(...) is used
 *                
 **********************************************************************/

template <typename T, typename U, typename V>
class GenericProcessor : public FairMQDevice, public T, public U, public V
{
    typedef T                                         deserialization_type;
    typedef U                                           serialization_type;
    typedef V                                               proc_task_type;
  public:
    GenericProcessor()
        : deserialization_type()
        , serialization_type()
        , proc_task_type()
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
        proc_task_type::InitTask(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void InitInputContainer(Args... args)
    {
        deserialization_type::InitContainer(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void InitOutputContainer(Args... args)
    {
        serialization_type::InitContainer(std::forward<Args>(args)...);
    }

    /*
     * 
    // ***********************  TODO: implement multipart features
    void SendPart()
    {
        fChannels["data-out"].at(0).Send(serialization_type::SerializeMsg(proc_task_type::GetData()), "snd-more");
        serialization_type::CloseMessage(); 
    }

    // void SendPart();
    // bool ReceivePart();
    bool ReceivePart()
    {
        if (fChannels["data-in"].at(0).ExpectsAnotherPart())
        {
            deserialization_type::CloseMessage(); 
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

        const FairMQChannel& inputChannel = fChannels["data-in"].at(0);
        const FairMQChannel& outputChannel = fChannels["data-out"].at(0);

        while (CheckCurrentState(RUNNING))
        {
            std::unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());

            ++receivedMsgs;

            if (inputChannel.Receive(msg) > 0)
            {
                // deserialization_type::DeSerializeMsg(msg) --> deserialize data of msg and fill output container
                // proc_task_type::ExecuteTask( ... )   --> process output container
                proc_task_type::ExecuteTask(deserialization_type::DeSerializeMsg(msg.get()));

                // serialization_type::fMessage point to msg
                serialization_type::SetMessage(msg.get());

                // proc_task_type::GetOutputData() --> Get processed output container
                // serialization_type::message(...)  --> Serialize output container and fill fMessage
                outputChannel.Send(serialization_type::SerializeMsg(proc_task_type::GetOutputData()));
                sentMsgs++;
            }
        }

        MQLOG(INFO) << "Received " << receivedMsgs << " and sent " << sentMsgs << " messages!";
    }

};

#endif /* GENERICPROCESSOR_H */


/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

/* 
 * File:   GenericFileSink.h
 * Author: winckler
 *
 * Created on October 7, 2014, 6:06 PM
 */

#ifndef GENERICFILESINK_H
#define	GENERICFILESINK_H

#include "FairMQDevice.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include "FairMQLogger.h"

/*********************************************************************
 * -------------- NOTES -----------------------
 * All policies must have a default constructor
 * Function to define in (parent) policy classes :
 * 
 *  -------- INPUT POLICY --------
 *                deserialization_type::InitContainer(...)
 * CONTAINER_TYPE deserialization_type::DeserializeMsg(FairMQMessage* msg)
 * 
 * 
 *  -------- OUTPUT POLICY --------
 *                sink_type::AddToFile(CONTAINER_TYPE);
 *                sink_type::InitOutputFile()
 **********************************************************************/

#include "FairMQDevice.h"

template <typename T, typename U>
class GenericFileSink : public FairMQDevice, public T, public U
{
  protected:
    typedef T                        deserialization_type;
    typedef U                                   sink_type;
  public:
    GenericFileSink()
        : deserialization_type()
        , sink_type()
    {}

    virtual ~GenericFileSink()
    {}

    template <typename... Args>
    void SetTransport(Args... args)
    {
        FairMQDevice::SetTransport(std::forward<Args>(args)...);
    }

    template <typename... Args>
    void InitInputContainer(Args... args)
    {
        deserialization_type::InitContainer(std::forward<Args>(args)...);
    }

  protected:
    virtual void InitTask()
    {
        sink_type::InitOutputFile();
    }

    virtual void Run()
    {
        int receivedMsg = 0;

        // store the channel reference to avoid traversing the map on every loop iteration
        const FairMQChannel& inputChannel = fChannels.at("data-in").at(0);

        while (CheckCurrentState(RUNNING))
        {
            std::unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());

            if (inputChannel.Receive(msg) > 0)
            {
                sink_type::AddToFile(deserialization_type::DeserializeMsg(msg.get()));
                receivedMsg++;
            }
        }

        MQLOG(INFO) << "Received " << receivedMsg << " messages!";
    }
};

#endif /* GENERICFILESINK_H */

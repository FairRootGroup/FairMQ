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
 *                InputPolicy::InitContainer(...)
 * CONTAINER_TYPE InputPolicy::DeSerializeMsg(FairMQMessage* msg)
 * 
 * 
 *  -------- OUTPUT POLICY --------
 *                OutputPolicy::AddToFile(CONTAINER_TYPE);
 *                OutputPolicy::InitOutputFile()
 **********************************************************************/



#include "FairMQDevice.h"

template <  typename InputPolicy, 
            typename OutputPolicy>
class GenericFileSink : public FairMQDevice, 
                        public InputPolicy, 
                        public OutputPolicy
{    
public:
    GenericFileSink(): 
        InputPolicy(),
        OutputPolicy()
    {}
        
    virtual ~GenericFileSink()
    {}
    
    
    void SetTransport(FairMQTransportFactory* transport)
    {
        FairMQDevice::SetTransport(transport);
    }

    template <typename... Args>
        void InitInputContainer(Args... args)
        {
            InputPolicy::InitContainer(std::forward<Args>(args)...);
        }
protected:

    virtual void Init()
    {
        FairMQDevice::Init();
        OutputPolicy::InitOutputFile();
    }

  protected:
    virtual void Run()
    {
        MQLOG(INFO) << ">>>>>>> Run <<<<<<<";

        boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

        int received = 0;
        int receivedMsg = 0;

        while (fState == RUNNING)
        {
            FairMQMessage* msg = fTransportFactory->CreateMessage();
            received = fPayloadInputs->at(0)->Receive(msg);
            if(received>0)
            {
                OutputPolicy::AddToFile(InputPolicy::DeSerializeMsg(msg));
                receivedMsg++;
            }
            delete msg;
        }

        MQLOG(INFO) << "Received " << receivedMsg << " messages!";
        try 
        {
            rateLogger.interrupt();
            rateLogger.join();
        } 
        catch(boost::thread_resource_error& e) 
        {
            MQLOG(ERROR) << e.what();
        }

        FairMQDevice::Shutdown();

        // notify parent thread about end of processing.
        boost::lock_guard<boost::mutex> lock(fRunningMutex);
        fRunningFinished = true;
        fRunningCondition.notify_one();
    }


};

#endif	/* GENERICFILESINK_H */


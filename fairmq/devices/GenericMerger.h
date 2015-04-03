/* 
 * File:   GenericMerger.h
 * Author: winckler
 *
 * Created on April 9, 2015, 1:37 PM
 */

#ifndef GENERICMERGER_H
#define	GENERICMERGER_H


#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "FairMQDevice.h"
#include "FairMQLogger.h"
#include "FairMQPoller.h"


template <  typename MergerPolicy, 
            typename InputPolicy, 
            typename OutputPolicy
         >
class GenericMerger :   public FairMQDevice, 
                        public MergerPolicy,
                        public InputPolicy, 
                        public OutputPolicy 
{
  public:
    GenericMerger() : fBlockingTime(100)
    {}
    
    virtual ~GenericMerger()
    {}

    void SetTransport(FairMQTransportFactory* transport)
    {
        FairMQDevice::SetTransport(transport);
    }
    
    
    
    
  protected:
      
    int fBlockingTime;
      
    
    virtual void Run()
    {
        MQLOG(INFO) << ">>>>>>> Run <<<<<<<";

        boost::thread rateLogger(boost::bind(&FairMQDevice::LogSocketRates, this));

        FairMQPoller* poller = fTransportFactory->CreatePoller(*fPayloadInputs);

        int received = 0;

        while (fState == RUNNING)
        {
            FairMQMessage* msg = fTransportFactory->CreateMessage();
            //MergerPolicy::
            poller->Poll(fBlockingTime);

            for (int i = 0; i < fNumInputs; i++)
            {
                if (poller->CheckInput(i))
                {
                    received = fPayloadInputs->at(i)->Receive(msg);
                    MergerPolicy::Merge(InputPolicy::DeSerializeMsg(msg));
                }
                
                OutputPolicy::SetMessage(msg);
                
                if ( received > 0 && MergerPolicy::ReadyToSend() )
                {
                    fPayloadOutputs->at(0)->Send(OutputPolicy::SerializeMsg(MergerPolicy::GetOutputData()));
                    received = 0;
                }
            }

            delete msg;
        }

        delete poller;

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

#endif	/* GENERICMERGER_H */


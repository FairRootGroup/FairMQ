/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/* 
 * File:   GenericSampler.h
 * Author: winckler
 *
 * Created on November 24, 2014, 3:30 PM
 */

#ifndef GENERICSAMPLER_H
#define GENERICSAMPLER_H

#include <vector>
#include <iostream>
#include <functional>
#include <stdint.h>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/timer/timer.hpp>

#include "FairMQDevice.h"
#include "FairMQLogger.h"
#include "FairMQTools.h"

/*  GENERIC SAMPLER (data source) MQ-DEVICE */
/********************************************************************* */
 
template<typename T, typename U>
using enable_if_match = typename std::enable_if<std::is_same<T,U>::value,int>::type;

struct DefaultSamplerRun {};

template <  typename T, 
            typename U,
            typename R=DefaultSamplerRun
         >
class base_GenericSampler : public FairMQDevice, public T, public U
{
  public:
    typedef T input_policy;// sampler source
    typedef U output_policy;// deserialization
    typedef R run_type;

    base_GenericSampler() : FairMQDevice(), fOutChanName("data-out"), T(), U()
    {}

    virtual ~base_GenericSampler()
    {}

    template <typename... Args>
    void SetFileProperties(Args&&... args)
    {
        input_policy::SetFileProperties(std::forward<Args>(args)...);
    }

  protected:

    
    using input_policy::fInput;
    virtual void Init()
    {    
        input_policy::InitSource();
        
    }

    template <typename RUN = run_type, enable_if_match<RUN,DefaultSamplerRun> = 0>
    inline void Run_impl()
    {
        int64_t sentMsgs(0);
        int64_t numEvents = input_policy::GetNumberOfEvent();
        LOG(INFO) << "Number of events to process: " << numEvents;
        boost::timer::auto_cpu_timer timer;
        for (int64_t idx(0); idx < numEvents; idx++)
        {
            std::unique_ptr<FairMQMessage> msg(NewMessage());
            T::Deserialize(idx);
            Send<U>(fInput, fOutChanName);
            sentMsgs++;
            if (!CheckCurrentState(RUNNING))
                break;
            
        }

        boost::timer::cpu_times const elapsed_time(timer.elapsed());
        LOG(INFO) << "Sent everything in:\n" << boost::timer::format(elapsed_time, 2);
        LOG(INFO) << "Sent " << sentMsgs << " messages!";
    }

    virtual void Run()
    {
        Run_impl();
    }

  private:
    std::string fOutChanName;

};


    void SendHeader(int /*socketIdx*/) {}




#endif /* GENERICSAMPLER_H */

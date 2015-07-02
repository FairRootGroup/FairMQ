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
#define	GENERICSAMPLER_H

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
/*********************************************************************
 * -------------- NOTES -----------------------
 * All policies must have a default constructor
 * Function to define in (parent) policy classes :
 * 
 *  -------- INPUT POLICY (SAMPLER POLICY) --------
 *                source_type::InitSampler()                          // must be there to compile
 *        int64_t source_type::GetNumberOfEvent()                     // must be there to compile
 *                source_type::SetIndex(int64_t eventIdx)             // must be there to compile
 * CONTAINER_TYPE source_type::GetOutData()                           // must be there to compile
 *                source_type::SetFileProperties(Args&... args)       // must be there to compile
 *                source_type::ExecuteTasks()                         // must be there to compile
 * 
 *           void BindSendPart(std::function<void(int)> callback)       // enabled if exists
 *           void BindGetSocketNumber(std::function<int()> callback)    // enabled if exists
 *           void BindGetCurrentIndex(std::function<int()> callback)    // enabled if exists
 * 
 *  -------- OUTPUT POLICY --------
 *                serialization_type::SerializeMsg(CONTAINER_TYPE)            // must be there to compile
 *                serialization_type::SetMessage(FairMQMessage* msg)          // must be there to compile
 *               
 **********************************************************************/

//template <typename source_type, typename serialization_type, typename key_type, typename task_type>
//class base_GenericSampler : public FairMQDevice, public source_type, public serialization_type
template <typename T, typename U, typename K, typename L>
class base_GenericSampler : public FairMQDevice, public T, public U
{
    typedef T                                                 source_type;
    typedef U                                          serialization_type;
    typedef K                                                    key_type;
    typedef L                                                   task_type;
    typedef base_GenericSampler<T,U,K,L>                        self_type;
    
  public:
    enum
    {
        EventRate = FairMQDevice::Last,
        OutChannelName
    };

    base_GenericSampler();
    virtual ~base_GenericSampler();
    /*
    struct trait : source_type::trait, serialization_type::trait
    {
        //static const SerializationTag serialization = serialization_type::trait::serialization;
        //static const FileTag 
        static const DeviceTag device = kSampler;
        typedef base_GenericSampler<source_type,serialization_type> self_type;
        typedef source_type source_type;
        typedef serialization_type serialization_type;
    };
    
    */

    virtual void SetTransport(FairMQTransportFactory* factory);
    void ResetEventCounter();

    template <typename... Args>
    void SetFileProperties(Args&... args)
    {
        source_type::SetFileProperties(args...);
    }

    virtual void SetProperty(const int key, const int value);
    virtual int GetProperty(const int key, const int default_ = 0);
    virtual void SetProperty(const int key, const std::string& value);
    virtual std::string GetProperty(const int key, const std::string& default_ = "");
    
    void SendPart(int socketIdx);
    int GetSocketNumber() const;
    int GetCurrentIndex() const;
    void SetContinuous(bool flag);
    
    
    /// ///////////////////////////////////////////////////////////////////////////////////////
    /*
       register the tasks you want to process and, which will be
       called by ExecuteTasks() member function. The registration is done by filling
       a std::map<key_type, task_type > where key_type is int and task_type
       is std::function<void()> by default (when using GenericSampler alias template). 
       The template parameter <RegistrationManager> must take a pointer to this class or derived class as first argument, 
       and a reference to a std::map<key_type, task_type > as second argument.
       It is convenient to use a lambda expression in the place of the <RegistrationManager> template argument.
       For example if we want to register the simple function,
       //<
       void myfunction() {std::cout << "hello World" << std::endl;} 
       //>
       ,and the MultiPartTask() template function member of this class , then
       we can do in the main function as follow :
       
       sampler.RegisterTask(
          [&](TSampler* s, std::map<int, std::function<void()> >& task_list) 
          {
              task_list[0]=std::bind(myfunction);
              task_list[1]=std::bind(&U::template MultiPartTask<5>, s);
          });
     
     To communicate with the Host derived class via callback, three methods from the host class are callable (only
     after binding these methods in the GenericSampler<I,O>::InitTask() )
     
    */
    template<typename RegistrationManager>
    void RegisterTask(RegistrationManager manage)
    {
        manage(this,fTaskList);
        LOG(DEBUG)<<"Current Number of registered tasks = "<<fTaskList.size();
    }
    /// ///////////////////////////////////////////////////////////////////////////////////////
    void ExecuteTasks()
    {
        for(const auto& p : fTaskList)
        {
            LOG(DEBUG)<<"Execute Task "<< p.first;
            p.second();
        }
    }
    
    
  protected:
    virtual void InitTask();
    virtual void Run();
    
  private:
    std::string fOutChanName;
    int64_t fNumEvents;
    int64_t fCurrentIdx;
    int fEventRate;
    int fEventCounter;
    bool fContinuous;
    std::map<key_type, task_type > fTaskList; // to handle Task list
    
    
    // automatically enable or disable the call of policy function members for binding of host functions.
    // this template functions use SFINAE to detect the existence of the policy function signature.
    template<typename S = source_type ,FairMQ::tools::enable_if_hasNot_BindSendPart<S> = 0 >
    void BindingSendPart(){}
    template<typename S = source_type ,FairMQ::tools::enable_if_has_BindSendPart<S> = 0 >
    void BindingSendPart()
    {
        source_type::BindSendPart(std::bind(&base_GenericSampler::SendPart,this,std::placeholders::_1) );
    }
    
    template<typename S = source_type ,FairMQ::tools::enable_if_hasNot_BindGetSocketNumber<S> = 0 >
    void BindingGetSocketNumber(){}
    template<typename S = source_type ,FairMQ::tools::enable_if_has_BindGetSocketNumber<S> = 0 >
    void BindingGetSocketNumber()
    {
        source_type::BindGetSocketNumber(std::bind(&base_GenericSampler::GetSocketNumber,this) );
    }
    
    template<typename S = source_type ,FairMQ::tools::enable_if_hasNot_BindGetCurrentIndex<S> = 0 >
    void BindingGetCurrentIndex(){}
    template<typename S = source_type ,FairMQ::tools::enable_if_has_BindGetCurrentIndex<S> = 0 >
    void BindingGetCurrentIndex()
    {
        source_type::BindGetCurrentIndex(std::bind(&base_GenericSampler::GetCurrentIndex,this) );
    }
};

#include "GenericSampler.tpl"

#endif /* GENERICSAMPLER_H */


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

template <  typename T/*=deserialization type*/, 
            typename U/*=serialization type*/, 
            typename V/*=task type*///, 
            //typename W/*=input creator type*/, 
            //typename X/*=output creator type*/
         >
class GenericProcessor :   public FairMQDevice, public T, public U,
                            public V//, 
                            //public W, 
                            //public X
{
    public:
    typedef T input_policy;
    typedef U output_policy;
    typedef V task_type;
    
    //typedef W input_creator_type;
    //typedef X output_creator_type;
    

  
    GenericProcessor()
        : FairMQDevice(), T(), U()
        , task_type()
        //, input_creator_type()
        //, output_creator_type()
    {}

    virtual ~GenericProcessor()
    {}

    template<typename... Args>
    void InitInputData(Args&&... args)
    {
        input_policy::Create(std::forward<Args>(args)...);
    }

    template<typename... Args>
    void InitOutputData(Args&&... args)
    {
        output_policy::Create(std::forward<Args>(args)...);
    }

  protected:
    using input_policy::fInput;
    using output_policy::fOutput;

    virtual void InitTask()
    {
    }

    virtual void Run()
    {
        int receivedMsgs = 0;
        int sentMsgs = 0;

        while (CheckCurrentState(RUNNING))
        {
            std::unique_ptr<FairMQMessage> msg(NewMessage());
            if (Receive(fInput, "data-in") > 0)
            {
                Deserialize<T>(*msg,fInput);
                receivedMsgs++;
                task_type::Exec(fInput,fOutput);

                Serialize<U>(*msg,fOutput);
                Send(fOutput, "data-out");
                sentMsgs++;
            }
        }
        LOG(INFO) << "Received " << receivedMsgs << " and sent " << sentMsgs << " messages!";
    }
};




#endif /* GENERICPROCESSOR_H */


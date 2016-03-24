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
  public:
    typedef T                        input_policy;
    typedef U                        sink_type;
    GenericFileSink()
        : FairMQDevice(), input_policy(), sink_type()
    {}

    virtual ~GenericFileSink()
    {}


    template<typename... Args>
    void InitInputData(Args&&... args)
    {
        input_policy::Create(std::forward<Args>(args)...);
    }

  protected:

    
    using input_policy::fInput;


    virtual void InitTask()
    {
        sink_type::InitOutputFile();
    }

    typedef typename input_policy::deserialization_type deserializer_type;
    virtual void Run()
    {
        int receivedMsg = 0;
        while (CheckCurrentState(RUNNING))
        {
            if (Receive<deserializer_type>(fInput, "data-in") > 0)
            {
                U::Serialize(fInput);// add fInput to file
                receivedMsg++;
            }
        }

        LOG(INFO) << "Received " << receivedMsg << " messages!";
    }
};

#endif /* GENERICFILESINK_H */

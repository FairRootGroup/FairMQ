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

template <typename InputPolicy, typename OutputPolicy>
class GenericFileSink : public FairMQDevice, public InputPolicy, public OutputPolicy
{
    //using InputPolicy::message;
    //using OutputPolicy::InitOutFile;
    //using OutputPolicy::AddToFile;
    
public:
    GenericFileSink();
    virtual ~GenericFileSink();
    
    template <typename... Args>
        void InitInputPolicyContainer(Args... args)
        {
            InputPolicy::InitContainer(std::forward<Args>(args)...);
        }
    
    
    virtual void SetTransport(FairMQTransportFactory* transport);
    virtual void InitOutputFile();
    
protected:
    virtual void Run();
    virtual void Init();
    
};

#include "GenericFileSink.tpl"

#endif	/* GENERICFILESINK_H */


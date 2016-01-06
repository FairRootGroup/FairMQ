/* 
 * File:   BaseDeserializationPolicy.h
 * Author: winckler
 *
 * Created on October 14, 2015, 1:01 PM
 */

#ifndef BASEDESERIALIZATIONPOLICY_H
#define BASEDESERIALIZATIONPOLICY_H

#include "FairMQMessage.h"

// c++11 code
#include <type_traits>

//  CRTP base class
template <typename TDerived >
class BaseDeserializationPolicy
{
  public:
    BaseDeserializationPolicy() {}

    virtual ~BaseDeserializationPolicy() {}

    template<typename C = TDerived>
    auto DeserializeMsg(FairMQMessage* msg)-> decltype(static_cast<C*>(this)->DeserializeMsg(msg))
    {
        static_assert(std::is_same<C, TDerived>{}, "BaseDeserializationPolicy::DeserializeMsg hack broken");
        return static_cast<TDerived*>(this)->DeserializeMsg(msg);
    }
};


/*
// c++14 code
//  CRTP base class
template <typename TDerived >
class BaseDeserializationPolicy
{
public:
    BaseDeserializationPolicy() 
    {}

    virtual ~BaseDeserializationPolicy()
    {}

    auto DeSerializeMsg(FairMQMessage* msg)
    {
    	return static_cast<TDerived*>(this)->DeSerializeMsg(msg);
    }

};*/

#endif /* BASEDESERIALIZATIONPOLICY_H */

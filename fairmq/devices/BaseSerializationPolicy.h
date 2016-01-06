/* 
 * File:   BaseSerializationPolicy.h
 * Author: winckler
 *
 * Created on October 14, 2015, 1:01 PM
 */

#ifndef BASESERIALIZATIONPOLICY_H
#define BASESERIALIZATIONPOLICY_H

#include "FairMQMessage.h"

#include <type_traits>
//  CRTP base class
template <typename TDerived >
class BaseSerializationPolicy
{
  public:
    BaseSerializationPolicy() {}

    virtual ~BaseSerializationPolicy() {}

    template<typename CONTAINER_TYPE, typename C = TDerived>
    auto SerializeMsg(CONTAINER_TYPE container) -> decltype(static_cast<C*>(this)->SerializeMsg(container) )
    {
        static_assert(std::is_same<C, TDerived>{}, "BaseSerializationPolicy::SerializeMsg hack broken");
        return static_cast<TDerived*>(this)->SerializeMsg(container);
    }

    template<typename C = TDerived>
    auto SetMessage(FairMQMessage* msg)-> decltype(static_cast<C*>(this)->SetMessage(msg) )
    {
        static_assert(std::is_same<C, TDerived>{}, "BaseSerializationPolicy::SetMessage hack broken");
        return static_cast<TDerived*>(this)->SetMessage(msg);
    }
};

/*
//  CRTP base class
// c++14 code
template <typename TDerived >
class BaseSerializationPolicy
{
public:
    BaseSerializationPolicy() 
    {}

    virtual ~BaseSerializationPolicy()
    {}

    template<typename CONTAINER_TYPE>
    auto SerializeMsg(CONTAINER_TYPE container)
    {
        return static_cast<TDerived*>(this)->SerializeMsg(container);
    }

    auto SetMessage(FairMQMessage* msg)
    {
        return static_cast<TDerived*>(this)->SetMessage(msg);
    }

};
*/

#endif /* BASESERIALIZATIONPOLICY_H */

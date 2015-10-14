/* 
 * File:   BaseProcessorTaskPolicy.h
 * Author: winckler
 *
 * Created on October 14, 2015, 1:01 PM
 */

#ifndef BASEPROCESSORTASKPOLICY_H
#define	BASEPROCESSORTASKPOLICY_H


#include <type_traits>
//  CRTP base class
template <typename TDerived >
class BaseProcessorTaskPolicy
{
public:
    BaseProcessorTaskPolicy() 
    {}

    virtual ~BaseProcessorTaskPolicy()
    {}

    template<typename C = TDerived>
    auto GetOutputData() -> decltype(static_cast<C*>(this)->GetOutputData() )
    {
    	static_assert(std::is_same<C, TDerived>{}, "BaseProcessorTaskPolicy::GetOutputData hack broken");
    	return static_cast<TDerived*>(this)->GetOutputData();
    }

    template<typename CONTAINER_TYPE, typename C = TDerived>
    auto ExecuteTask(CONTAINER_TYPE container) -> decltype( static_cast<C*>(this)->ExecuteTask(container) )
    {
    	static_assert(std::is_same<C, TDerived>{}, "BaseProcessorTaskPolicy::ExecuteTask hack broken");
    	return static_cast<TDerived*>(this)->ExecuteTask(container);
    }


};

 /*
  
// c++14 code only
//  CRTP base class
template <typename TDerived >
class BaseProcessorTaskPolicy
{
public:
    BaseProcessorTaskPolicy() 
    {}

    virtual ~BaseProcessorTaskPolicy()
    {}

    auto GetOutputData()
    {
    	return static_cast<TDerived*>(this)->GetOutputData();
    }

    template<typename CONTAINER_TYPE>
    auto ExecuteTask(CONTAINER_TYPE container)
    {
    	return static_cast<TDerived*>(this)->ExecuteTask(container);
    }

};
*/
#endif	/* BASEPROCESSORTASKPOLICY_H */


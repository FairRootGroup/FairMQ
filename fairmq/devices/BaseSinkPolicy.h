/* 
 * File:   BaseSinkPolicy.h
 * Author: winckler
 *
 * Created on October 14, 2015, 1:01 PM
 */

#ifndef BASESINKPOLICY_H
#define	BASESINKPOLICY_H


#include <type_traits>
//  CRTP base class
template <typename TDerived >
class BaseSinkPolicy
{
public:
    BaseSinkPolicy() 
    {}

    virtual ~BaseSinkPolicy()
    {}

    template<typename CONTAINER_TYPE, typename C = TDerived>
    auto AddToFile(CONTAINER_TYPE container) -> decltype(static_cast<C*>(this)->AddToFile(container) )
    {
        static_assert(std::is_same<C, TDerived>{}, "BaseSinkPolicy::AddToFile hack broken");
        return static_cast<TDerived*>(this)->AddToFile(container);
    }

    template<typename C = TDerived>
    auto InitOutputFile() -> decltype(static_cast<C*>(this)->InitOutputFile() )
    {
        static_assert(std::is_same<C, TDerived>{}, "BaseSinkPolicy::InitOutputFile hack broken");
        return static_cast<TDerived*>(this)->InitOutputFile();
    }

};

#endif	/* BASESINKPOLICY_H */


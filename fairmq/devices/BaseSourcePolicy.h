/* 
 * File:   BaseSourcePolicy.h
 * Author: winckler
 *
 * Created on October 14, 2015, 1:01 PM
 */

#ifndef BASESOURCEPOLICY_H
#define BASESOURCEPOLICY_H

#include <type_traits>

// c++11 code
//  CRTP base class
template <typename TDerived >
class BaseSourcePolicy
{
  public:
    BaseSourcePolicy() {}

    virtual ~BaseSourcePolicy() {}

    template<typename C = TDerived>
    auto InitSource()-> decltype(static_cast<C*>(this)->InitSource() )
    {
        static_assert(std::is_same<C, TDerived>{}, "BaseSourcePolicy::InitSource hack broken");
        return static_cast<TDerived*>(this)->InitSource();
    }

    template<typename C = TDerived>
    int64_t GetNumberOfEvent()//-> decltype(static_cast<C*>(this)->GetNumberOfEvent() )
    {
        static_assert(std::is_same<C, TDerived>{}, "BaseSourcePolicy::GetNumberOfEvent hack broken");
        return static_cast<TDerived*>(this)->GetNumberOfEvent();
    }

    template<typename C = TDerived>
    auto SetIndex(int64_t eventIdx)-> decltype(static_cast<C*>(this)->SetIndex(eventIdx) )
    {
        static_assert(std::is_same<C, TDerived>{}, "BaseSourcePolicy::SetIndex hack broken");
        return static_cast<TDerived*>(this)->SetIndex(eventIdx);
    }

    template<typename C = TDerived>
    //auto GetOutData()-> decltype(static_cast<C*>(this)->GetOutData() )
    decltype(std::declval<C*>()->GetOutData() ) GetOutData()
    {
        static_assert(std::is_same<C, TDerived>{}, "BaseSourcePolicy::GetOutData hack broken");
        return static_cast<TDerived*>(this)->GetOutData();
    }
};

/*
// c++14 code
//  CRTP base class
template <typename TDerived >
class BaseSourcePolicy
{
public:
    BaseSourcePolicy() 
    {}

    virtual ~BaseSourcePolicy()
    {}

    auto InitSource()
    {
        return static_cast<TDerived*>(this)->InitSource();
    }

    int64_t GetNumberOfEvent()
    {
        return static_cast<TDerived*>(this)->GetNumberOfEvent();
    }

    auto SetIndex(int64_t eventIdx)
    {
        return static_cast<TDerived*>(this)->SetIndex(int64_t eventIdx);
    }

    auto GetOutData()
    {
        return static_cast<TDerived*>(this)->GetOutData();
    }

};
*/

#endif /* BASESOURCEPOLICY_H */

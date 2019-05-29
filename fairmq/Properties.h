/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_PROPERTIES_H
#define FAIR_MQ_PROPERTIES_H

#include <fairmq/EventManager.h>

#include <boost/any.hpp>
#include <boost/core/demangle.hpp>

#include <functional>
#include <map>
#include <unordered_map>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <utility> // pair

namespace fair
{
namespace mq
{

using Property = boost::any;
using Properties = std::map<std::string, Property>;

struct PropertyChange : Event<std::string> {};
struct PropertyChangeAsString : Event<std::string> {};

class PropertyHelper
{
  public:
    template<typename T>
    static void AddType(std::string label = "")
    {
        if (label == "") {
            label = boost::core::demangle(typeid(T).name());
        }
        fTypeInfos[std::type_index(typeid(T))] = [label](const fair::mq::Property& p) {
            std::stringstream ss;
            ss << boost::any_cast<T>(p);
            return std::pair<std::string, std::string>{ss.str(), label};
        };
    }

    static std::string ConvertPropertyToString(const Property& p)
    {
        return fTypeInfos.at(p.type())(p).first;
    }

    // returns <valueAsString, typenameAsString>
    static std::pair<std::string, std::string> GetPropertyInfo(const Property& p)
    {
        try {
            return fTypeInfos.at(p.type())(p);
        } catch (std::out_of_range& oor) {
            return {"[unidentified_type]", "[unidentified_type]"};
        }
    }

    static std::unordered_map<std::type_index, void(*)(const fair::mq::EventManager&, const std::string&, const fair::mq::Property&)> fEventEmitters;
  private:
    static std::unordered_map<std::type_index, std::function<std::pair<std::string, std::string>(const fair::mq::Property&)>> fTypeInfos;
};

}
}

#endif /* FAIR_MQ_PROPERTIES_H */

/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/*
 * File:   FairProgOptionsHelper.h
 * Author: winckler
 *
 * Created on March 11, 2015, 5:38 PM
 */

#ifndef FAIRPROGOPTIONSHELPER_H
#define FAIRPROGOPTIONSHELPER_H

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/spirit/home/support/detail/hold_any.hpp>

#include <string>
#include <vector>
#include <iostream>
#include <ostream>
#include <iterator>
#include <typeinfo>

namespace fair
{
namespace mq
{

template<class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    for (const auto& i : v)
    {
        os << i << "  ";
    }
    return os;
}

struct VarValInfo
{
    std::string value;
    std::string type;
    std::string defaulted;
};

template<typename T>
std::string ConvertVariableValueToString(const boost::program_options::variable_value& varVal)
{
    std::ostringstream oss;
    if (auto q = boost::any_cast<T>(&varVal.value())) {
        oss << *q;
    }
    return oss.str();
}

namespace options
{

// policy to convert boost variable value into string
struct ToString
{
    using returned_type = std::string;

    template<typename T>
    std::string Value(const boost::program_options::variable_value& varVal, const std::string&, const std::string&)
    {
        return ConvertVariableValueToString<T>(varVal);
    }

    returned_type DefaultValue(const std::string&)
    {
        return std::string("[unidentified]");
    }
};

// policy to convert variable value content into VarValInfo
struct ToVarValInfo
{
    using returned_type = VarValInfo;

    template<typename T>
    returned_type Value(const boost::program_options::variable_value& varVal, const std::string& type, const std::string& defaulted)
    {
        return VarValInfo{ConvertVariableValueToString<T>(varVal), type, defaulted};
    }

    returned_type DefaultValue(const std::string& defaulted)
    {
        return VarValInfo{std::string("[unidentified]"), std::string("[unidentified]"), defaulted};
    }
};

} // namespace options

// host class that take one of the two policy defined above
template<typename T>
struct ConvertVariableValue : T
{
    auto operator()(const boost::program_options::variable_value& varVal) -> typename T::returned_type
    {
        std::string defaulted;

        if (varVal.defaulted())
        {
            defaulted = " [default]";
        }
        else
        {
            defaulted = " [provided]";
        }

        if (typeid(std::string) == varVal.value().type())
            return T::template Value<std::string>(varVal, std::string("<string>"), defaulted);

        if (typeid(std::vector<std::string>) == varVal.value().type())
            return T::template Value<std::vector<std::string>>(varVal, std::string("<vector<string>>"), defaulted);

        if (typeid(int) == varVal.value().type())
            return T::template Value<int>(varVal, std::string("<int>"), defaulted);

        if (typeid(std::vector<int>) == varVal.value().type())
            return T::template Value<std::vector<int>>(varVal, std::string("<vector<int>>"), defaulted);

        if (typeid(float) == varVal.value().type())
            return T::template Value<float>(varVal, std::string("<float>"), defaulted);

        if (typeid(std::vector<float>) == varVal.value().type())
            return T::template Value<std::vector<float>>(varVal, std::string("<vector<float>>"), defaulted);

        if (typeid(double) == varVal.value().type())
            return T::template Value<double>(varVal, std::string("<double>"), defaulted);

        if (typeid(std::vector<double>) == varVal.value().type())
            return T::template Value<std::vector<double>>(varVal, std::string("<vector<double>>"), defaulted);

        if (typeid(short) == varVal.value().type())
            return T::template Value<short>(varVal, std::string("<short>"), defaulted);

        if (typeid(std::vector<short>) == varVal.value().type())
            return T::template Value<std::vector<short>>(varVal, std::string("<vector<short>>"), defaulted);

        if (typeid(long) == varVal.value().type())
            return T::template Value<long>(varVal, std::string("<long>"), defaulted);

        if (typeid(std::vector<long>) == varVal.value().type())
            return T::template Value<std::vector<long>>(varVal, std::string("<vector<long>>"), defaulted);

        if (typeid(std::size_t) == varVal.value().type())
            return T::template Value<std::size_t>(varVal, std::string("<std::size_t>"), defaulted);

        if (typeid(std::vector<std::size_t>) == varVal.value().type())
            return T::template Value<std::vector<std::size_t>>(varVal, std::string("<vector<std::size_t>>"), defaulted);

        if (typeid(std::uint32_t) == varVal.value().type())
            return T::template Value<std::uint32_t>(varVal, std::string("<std::uint32_t>"), defaulted);

        if (typeid(std::vector<std::uint32_t>) == varVal.value().type())
            return T::template Value<std::vector<std::uint32_t>>(varVal, std::string("<vector<std::uint32_t>>"), defaulted);

        if (typeid(std::uint64_t) == varVal.value().type())
            return T::template Value<std::uint64_t>(varVal, std::string("<std::uint64_t>"), defaulted);

        if (typeid(std::vector<std::uint64_t>) == varVal.value().type())
            return T::template Value<std::vector<std::uint64_t>>(varVal, std::string("<vector<std::uint64_t>>"), defaulted);

        if (typeid(bool) == varVal.value().type())
            return T::template Value<bool>(varVal, std::string("<bool>"), defaulted);

        if (typeid(std::vector<bool>) == varVal.value().type())
            return T::template Value<std::vector<bool>>(varVal, std::string("<vector<bool>>"), defaulted);

        if (typeid(boost::filesystem::path) == varVal.value().type())
            return T::template Value<boost::filesystem::path>(varVal, std::string("<boost::filesystem::path>"), defaulted);

        if (typeid(std::vector<boost::filesystem::path>) == varVal.value().type())
            return T::template Value<std::vector<boost::filesystem::path>>(varVal, std::string("<std::vector<boost::filesystem::path>>"), defaulted);

        // if we get here, the type is not supported return unknown info
        return T::DefaultValue(defaulted);
    }
};

} // namespace mq
} // namespace fair

#endif /* FAIRPROGOPTIONSHELPER_H */

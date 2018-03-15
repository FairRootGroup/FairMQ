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

#include <string>
#include <vector>
#include <iostream>
#include <ostream>
#include <iterator>

namespace fair
{
namespace mq
{

struct VarValInfo
{
    std::string value;
    std::string type;
    std::string defaulted;
    std::string empty;
};

template<class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    std::copy(v.begin(), v.end(), std::ostream_iterator<T>(os, "  "));
    return os;
}

template<typename T>
bool typeIs(const boost::program_options::variable_value& varValue)
{
    auto& value = varValue.value();
    if (auto q = boost::any_cast<T>(&value))
    {
        return true;
    }
    else
    {
        return false;
    }
}

template<typename T>
std::string ConvertVariableValueToString(const boost::program_options::variable_value& varValue)
{
    auto& value = varValue.value();
    std::ostringstream ostr;
    if (auto q = boost::any_cast<T>(&value))
    {
        ostr << *q;
    }
    std::string valueStr = ostr.str();
    return valueStr;
}

// string specialization
template<>
inline std::string ConvertVariableValueToString<std::string>(const boost::program_options::variable_value& varValue)
{
    auto& value = varValue.value();
    std::string valueStr;
    if (auto q = boost::any_cast<std::string>(&value))
    {
        valueStr = *q;
    }
    return valueStr;
}

// boost::filesystem::path specialization
template<>
inline std::string ConvertVariableValueToString<boost::filesystem::path>(const boost::program_options::variable_value& varValue)
{
    auto& value = varValue.value();
    std::string valueStr;
    if (auto q = boost::any_cast<boost::filesystem::path>(&value))
    {
        valueStr = (*q).string();
    }
    return valueStr;
}

// policy to convert boost variable value into string
struct VarInfoToString
{
    using returned_type = std::string;

    template<typename T>
    std::string Value(const boost::program_options::variable_value& varValue, const std::string&, const std::string&, const std::string&)
    {
        return ConvertVariableValueToString<T>(varValue);
    }

    returned_type DefaultValue(const std::string&, const std::string&)
    {
        return std::string("empty value");
    }
};

// policy to convert variable value content into VarValInfo
struct ToVarValInfo
{
    using returned_type = fair::mq::VarValInfo;

    template<typename T>
    returned_type Value(const boost::program_options::variable_value& varValue, const std::string& type, const std::string& defaulted, const std::string& empty)
    {
        std::string valueStr = ConvertVariableValueToString<T>(varValue);
        return fair::mq::VarValInfo{valueStr, type, defaulted, empty};
    }

    returned_type DefaultValue(const std::string& defaulted, const std::string& empty)
    {
        return fair::mq::VarValInfo{std::string("Unknown value"), std::string(" [Unknown]"), defaulted, empty};
    }
};

// host class that take one of the two policy defined above
template<typename T>
struct ConvertVariableValue : T
{
    auto operator()(const boost::program_options::variable_value& varValue) -> typename T::returned_type
    {
        std::string defaulted;
        std::string empty;

        if (varValue.empty())
        {
            empty = " [empty]";
        }
        else
        {
            if (varValue.defaulted())
            {
                defaulted = " [default]";
            }
            else
            {
                defaulted = " [provided]";
            }
        }

        if (typeIs<std::string>(varValue))
            return T::template Value<std::string>(varValue, std::string("<string>"), defaulted, empty);

        if (typeIs<std::vector<std::string>>(varValue))
            return T::template Value<std::vector<std::string>>(varValue, std::string("<vector<string>>"), defaulted, empty);

        if (typeIs<int>(varValue))
            return T::template Value<int>(varValue, std::string("<int>"), defaulted, empty);

        if (typeIs<std::vector<int>>(varValue))
            return T::template Value<std::vector<int>>(varValue, std::string("<vector<int>>"), defaulted, empty);

        if (typeIs<float>(varValue))
            return T::template Value<float>(varValue, std::string("<float>"), defaulted, empty);

        if (typeIs<std::vector<float>>(varValue))
            return T::template Value<std::vector<float>>(varValue, std::string("<vector<float>>"), defaulted, empty);

        if (typeIs<double>(varValue))
            return T::template Value<double>(varValue, std::string("<double>"), defaulted, empty);

        if (typeIs<std::vector<double>>(varValue))
            return T::template Value<std::vector<double>>(varValue, std::string("<vector<double>>"), defaulted, empty);

        if (typeIs<short>(varValue))
            return T::template Value<short>(varValue, std::string("<short>"), defaulted, empty);

        if (typeIs<std::vector<short>>(varValue))
            return T::template Value<std::vector<short>>(varValue, std::string("<vector<short>>"), defaulted, empty);

        if (typeIs<long>(varValue))
            return T::template Value<long>(varValue, std::string("<long>"), defaulted, empty);

        if (typeIs<std::vector<long>>(varValue))
            return T::template Value<std::vector<long>>(varValue, std::string("<vector<long>>"), defaulted, empty);

        if (typeIs<std::size_t>(varValue))
            return T::template Value<std::size_t>(varValue, std::string("<std::size_t>"), defaulted, empty);

        if (typeIs<std::vector<std::size_t>>(varValue))
            return T::template Value<std::vector<std::size_t>>(varValue, std::string("<vector<std::size_t>>"), defaulted, empty);

        if (typeIs<std::uint32_t>(varValue))
            return T::template Value<std::uint32_t>(varValue, std::string("<std::uint32_t>"), defaulted, empty);

        if (typeIs<std::vector<std::uint32_t>>(varValue))
            return T::template Value<std::vector<std::uint32_t>>(varValue, std::string("<vector<std::uint32_t>>"), defaulted, empty);

        if (typeIs<std::uint64_t>(varValue))
            return T::template Value<std::uint64_t>(varValue, std::string("<std::uint64_t>"), defaulted, empty);

        if (typeIs<std::vector<std::uint64_t>>(varValue))
            return T::template Value<std::vector<std::uint64_t>>(varValue, std::string("<vector<std::uint64_t>>"), defaulted, empty);

        if (typeIs<bool>(varValue))
            return T::template Value<bool>(varValue, std::string("<bool>"), defaulted, empty);

        if (typeIs<std::vector<bool>>(varValue))
            return T::template Value<std::vector<bool>>(varValue, std::string("<vector<bool>>"), defaulted, empty);

        if (typeIs<boost::filesystem::path>(varValue))
            return T::template Value<boost::filesystem::path>(varValue, std::string("<boost::filesystem::path>"), defaulted, empty);

        // if we get here, the type is not supported return unknown info
        return T::DefaultValue(defaulted, empty);
    }
};

} // namespace mq
} // namespace fair

#endif /* FAIRPROGOPTIONSHELPER_H */

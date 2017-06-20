/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
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
#include <fstream>
#include <iterator>
#include <tuple>

template<class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    std::copy(v.begin(), v.end(), std::ostream_iterator<T>(os, "  "));
    return os;
}

namespace FairMQ
{

namespace po = boost::program_options;

template<typename T>
bool is_this_type(const po::variable_value& varValue)
{
    auto& value = varValue.value();
    if (auto q = boost::any_cast<T>(&value))
        return true;
    else
        return false;
}

template<typename T>
std::string ConvertVariableValueToString(const po::variable_value& varValue)
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
inline std::string ConvertVariableValueToString<std::string>(const po::variable_value& varValue)
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
inline std::string ConvertVariableValueToString<boost::filesystem::path>(const po::variable_value& varValue)
{
    auto& value = varValue.value();
    std::string valueStr;
    if (auto q = boost::any_cast<boost::filesystem::path>(&value))
    {
        valueStr = (*q).string();
    }
    return valueStr;
}

#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif

// policy to convert boost variable value into string
struct ToString
{
    typedef std::string returned_type;
    template<typename T>
    std::string Value(const po::variable_value& varValue, const std::string&, const std::string&, const std::string&)
    {
        return ConvertVariableValueToString<T>(varValue);
    }

    returned_type DefaultValue(const std::string&, const std::string&)
    {
        return std::string("empty value");
    }
};

// policy to convert variable value content into a tuple with value, type, defaulted, empty information
struct ToVarInfo
{
    typedef std::tuple<std::string, std::string,std::string, std::string> returned_type;
    template<typename T>
    returned_type Value(const po::variable_value& varValue, const std::string& type, const std::string& defaulted, const std::string& empty)
    {
        std::string valueStr = ConvertVariableValueToString<T>(varValue);
        return make_tuple(valueStr, type, defaulted, empty);
    }

    returned_type DefaultValue(const std::string& defaulted, const std::string& empty)
    {
        return make_tuple(std::string("Unknown value"), std::string(" [Unknown]"), defaulted, empty);
    }
};

// host class that take one of the two policy defined above
template<typename T>
struct ConvertVariableValue : T
{
    //typename T::returned_type Run(const po::variable_value& varValue) //-> decltype(T::returned_type)
    auto Run(const po::variable_value& varValue) -> typename T::returned_type
    {
        std::string defaultedValue;
        std::string emptyValue;

        if (varValue.empty())
        {
            emptyValue = " [empty]";
        }
        else
        {
            if (varValue.defaulted())
            {
                defaultedValue = " [default]";
            }
            else
            {
                defaultedValue = " [provided]";
            }
        }

        // emptyValue += " *";

        //////////////////////////////// std types
        // std::string albeit useless here
        if (is_this_type<std::string>(varValue))
            return T::template Value<std::string>(varValue, std::string("<string>"), defaultedValue, emptyValue);

        // std::vector<std::string>
        if (is_this_type<std::vector<std::string>>(varValue))
            return T::template Value<std::vector<std::string>>(varValue, std::string("<vector<string>>"), defaultedValue, emptyValue);

        // int
        if (is_this_type<int>(varValue))
            return T::template Value<int>(varValue, std::string("<int>"), defaultedValue, emptyValue);

        // std::vector<int>
        if (is_this_type<std::vector<int>>(varValue))
            return T::template Value<std::vector<int>>(varValue, std::string("<vector<int>>"), defaultedValue, emptyValue);

        // float
        if (is_this_type<float>(varValue))
            return T::template Value<float>(varValue, std::string("<float>"), defaultedValue, emptyValue);

        // std::vector float
        if (is_this_type<std::vector<float>>(varValue))
            return T::template Value<std::vector<float>>(varValue, std::string("<vector<float>>"), defaultedValue, emptyValue);

        // double
        if (is_this_type<double>(varValue))
            return T::template Value<double>(varValue, std::string("<double>"), defaultedValue, emptyValue);

        // std::vector double
        if (is_this_type<std::vector<double>>(varValue))
            return T::template Value<std::vector<double>>(varValue, std::string("<vector<double>>"), defaultedValue, emptyValue);

        // short
        if (is_this_type<short>(varValue))
            return T::template Value<short>(varValue, std::string("<short>"), defaultedValue, emptyValue);

        // std::vector short
        if (is_this_type<std::vector<short>>(varValue))
            return T::template Value<std::vector<short>>(varValue, std::string("<vector<short>>"), defaultedValue, emptyValue);

        // long
        if (is_this_type<long>(varValue))
            return T::template Value<long>(varValue, std::string("<long>"), defaultedValue, emptyValue);

        // std::vector short
        if (is_this_type<std::vector<long>>(varValue))
            return T::template Value<std::vector<long>>(varValue, std::string("<vector<long>>"), defaultedValue, emptyValue);

        // size_t
        if (is_this_type<std::size_t>(varValue))
            return T::template Value<std::size_t>(varValue, std::string("<std::size_t>"), defaultedValue, emptyValue);

        // std::vector size_t
        if (is_this_type<std::vector<std::size_t>>(varValue))
            return T::template Value<std::vector<std::size_t>>(varValue, std::string("<vector<std::size_t>>"), defaultedValue, emptyValue);

        // uint64_t
        if (is_this_type<std::uint64_t>(varValue))
            return T::template Value<std::uint64_t>(varValue, std::string("<std::uint64_t>"), defaultedValue, emptyValue);

        // std::vector uint64_t
        if (is_this_type<std::vector<std::uint64_t>>(varValue))
            return T::template Value<std::vector<std::uint64_t>>(varValue, std::string("<vector<std::uint64_t>>"), defaultedValue, emptyValue);

        // bool
        if (is_this_type<bool>(varValue))
            return T::template Value<bool>(varValue, std::string("<bool>"), defaultedValue, emptyValue);

        // std::vector bool
        if (is_this_type<std::vector<bool>>(varValue))
            return T::template Value<std::vector<bool>>(varValue, std::string("<vector<bool>>"), defaultedValue, emptyValue);

        //////////////////////////////// boost types
        // boost::filesystem::path
        if (is_this_type<boost::filesystem::path>(varValue))
            return T::template Value<boost::filesystem::path>(varValue, std::string("<boost::filesystem::path>"), defaultedValue, emptyValue);

        // if we get here, the type is not supported return unknown info
        return T::DefaultValue(defaultedValue, emptyValue);
    }
};

#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

} // FairMQ namespace

#endif /* FAIRPROGOPTIONSHELPER_H */

/********************************************************************************
 * Copyright (C) 2014-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/Properties.h>
#include <fairmq/Tools.h>

#include <boost/filesystem.hpp>

using namespace std;
using namespace fair::mq::tools;
using boost::any_cast;

namespace fair
{
namespace mq
{

template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    for (unsigned int i = 0; i < v.size(); ++i) {
        os << v[i];
        if (i != v.size() - 1) {
            os << ", ";
        }
    }
    return os;
}

template<typename T>
pair<string, string> getString(const boost::any& v, const string& label)
{
    return { to_string(any_cast<T>(v)), label };
}


template<typename T>
pair<string, string> getStringPair(const boost::any& v, const string& label)
{
    stringstream ss;
    ss << any_cast<T>(v);
    return { ss.str(), label };
}

unordered_map<type_index, function<pair<string, string>(const Property&)>> PropertyHelper::fTypeInfos = {
    { type_index(typeid(char)),                            [](const Property& p) { return pair<string, string>{ string(1, any_cast<char>(p)), "char" }; } },
    { type_index(typeid(unsigned char)),                   [](const Property& p) { return pair<string, string>{ string(1, any_cast<unsigned char>(p)), "unsigned char" }; } },
    { type_index(typeid(string)),                          [](const Property& p) { return pair<string, string>{ any_cast<string>(p), "string" }; } },
    { type_index(typeid(int)),                             [](const Property& p) { return getString<int>(p, "int"); } },
    { type_index(typeid(size_t)),                          [](const Property& p) { return getString<size_t>(p, "size_t"); } },
    { type_index(typeid(uint32_t)),                        [](const Property& p) { return getString<uint32_t>(p, "uint32_t"); } },
    { type_index(typeid(uint64_t)),                        [](const Property& p) { return getString<uint64_t>(p, "uint64_t"); } },
    { type_index(typeid(long)),                            [](const Property& p) { return getString<long>(p, "long"); } },
    { type_index(typeid(long long)),                       [](const Property& p) { return getString<long long>(p, "long long"); } },
    { type_index(typeid(unsigned)),                        [](const Property& p) { return getString<unsigned>(p, "unsigned"); } },
    { type_index(typeid(unsigned long)),                   [](const Property& p) { return getString<unsigned long>(p, "unsigned long"); } },
    { type_index(typeid(unsigned long long)),              [](const Property& p) { return getString<unsigned long long>(p, "unsigned long long"); } },
    { type_index(typeid(float)),                           [](const Property& p) { return getString<float>(p, "float"); } },
    { type_index(typeid(double)),                          [](const Property& p) { return getString<double>(p, "double"); } },
    { type_index(typeid(long double)),                     [](const Property& p) { return getString<long double>(p, "long double"); } },
    { type_index(typeid(bool)),                            [](const Property& p) { stringstream ss; ss << boolalpha << any_cast<bool>(p); return pair<string, string>{ ss.str(), "bool" }; } },
    { type_index(typeid(vector<bool>)),                    [](const Property& p) { stringstream ss; ss << boolalpha << any_cast<vector<bool>>(p); return pair<string, string>{ ss.str(), "vector<bool>>" }; } },
    { type_index(typeid(boost::filesystem::path)),         [](const Property& p) { return getStringPair<boost::filesystem::path>(p, "boost::filesystem::path"); } },
    { type_index(typeid(vector<char>)),                    [](const Property& p) { return getStringPair<vector<char>>(p, "vector<char>"); } },
    { type_index(typeid(vector<unsigned char>)),           [](const Property& p) { return getStringPair<vector<unsigned char>>(p, "vector<unsigned char>"); } },
    { type_index(typeid(vector<string>)),                  [](const Property& p) { return getStringPair<vector<string>>(p, "vector<string>"); } },
    { type_index(typeid(vector<int>)),                     [](const Property& p) { return getStringPair<vector<int>>(p, "vector<int>"); } },
    { type_index(typeid(vector<size_t>)),                  [](const Property& p) { return getStringPair<vector<size_t>>(p, "vector<size_t>"); } },
    { type_index(typeid(vector<uint32_t>)),                [](const Property& p) { return getStringPair<vector<uint32_t>>(p, "vector<uint32_t>"); } },
    { type_index(typeid(vector<uint64_t>)),                [](const Property& p) { return getStringPair<vector<uint64_t>>(p, "vector<uint64_t>"); } },
    { type_index(typeid(vector<long>)),                    [](const Property& p) { return getStringPair<vector<long>>(p, "vector<long>"); } },
    { type_index(typeid(vector<long long>)),               [](const Property& p) { return getStringPair<vector<long long>>(p, "vector<long long>"); } },
    { type_index(typeid(vector<unsigned>)),                [](const Property& p) { return getStringPair<vector<unsigned>>(p, "vector<unsigned>"); } },
    { type_index(typeid(vector<unsigned long>)),           [](const Property& p) { return getStringPair<vector<unsigned long>>(p, "vector<unsigned long>"); } },
    { type_index(typeid(vector<unsigned long long>)),      [](const Property& p) { return getStringPair<vector<unsigned long long>>(p, "vector<unsigned long long>"); } },
    { type_index(typeid(vector<float>)),                   [](const Property& p) { return getStringPair<vector<float>>(p, "vector<float>"); } },
    { type_index(typeid(vector<double>)),                  [](const Property& p) { return getStringPair<vector<double>>(p, "vector<double>"); } },
    { type_index(typeid(vector<long double>)),             [](const Property& p) { return getStringPair<vector<long double>>(p, "vector<long double>"); } },
    { type_index(typeid(vector<boost::filesystem::path>)), [](const Property& p) { return getStringPair<vector<boost::filesystem::path>>(p, "vector<boost::filesystem::path>"); } },
};

unordered_map<type_index, void(*)(const EventManager&, const string&, const Property&)> PropertyHelper::fEventEmitters = {
    { type_index(typeid(char)),                            [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, char>(k, any_cast<char>(p)); } },
    { type_index(typeid(unsigned char)),                   [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, unsigned char>(k, any_cast<unsigned char>(p)); } },
    { type_index(typeid(string)),                          [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, string>(k, any_cast<string>(p)); } },
    { type_index(typeid(int)),                             [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, int>(k, any_cast<int>(p)); } },
    { type_index(typeid(size_t)),                          [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, size_t>(k, any_cast<size_t>(p)); } },
    { type_index(typeid(uint32_t)),                        [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, uint32_t>(k, any_cast<uint32_t>(p)); } },
    { type_index(typeid(uint64_t)),                        [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, uint64_t>(k, any_cast<uint64_t>(p)); } },
    { type_index(typeid(long)),                            [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, long>(k, any_cast<long>(p)); } },
    { type_index(typeid(long long)),                       [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, long long>(k, any_cast<long long>(p)); } },
    { type_index(typeid(unsigned)),                        [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, unsigned>(k, any_cast<unsigned>(p)); } },
    { type_index(typeid(unsigned long)),                   [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, unsigned long>(k, any_cast<unsigned long>(p)); } },
    { type_index(typeid(unsigned long long)),              [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, unsigned long long>(k, any_cast<unsigned long long>(p)); } },
    { type_index(typeid(float)),                           [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, float>(k, any_cast<float>(p)); } },
    { type_index(typeid(double)),                          [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, double>(k, any_cast<double>(p)); } },
    { type_index(typeid(long double)),                     [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, long double>(k, any_cast<long double>(p)); } },
    { type_index(typeid(bool)),                            [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, bool>(k, any_cast<bool>(p)); } },
    { type_index(typeid(vector<bool>)),                    [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<bool>>(k, any_cast<vector<bool>>(p)); } },
    { type_index(typeid(boost::filesystem::path)),         [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, boost::filesystem::path>(k, any_cast<boost::filesystem::path>(p)); } },
    { type_index(typeid(vector<char>)),                    [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<char>>(k, any_cast<vector<char>>(p)); } },
    { type_index(typeid(vector<unsigned char>)),           [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<unsigned char>>(k, any_cast<vector<unsigned char>>(p)); } },
    { type_index(typeid(vector<string>)),                  [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<string>>(k, any_cast<vector<string>>(p)); } },
    { type_index(typeid(vector<int>)),                     [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<int>>(k, any_cast<vector<int>>(p)); } },
    { type_index(typeid(vector<size_t>)),                  [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<size_t>>(k, any_cast<vector<size_t>>(p)); } },
    { type_index(typeid(vector<uint32_t>)),                [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<uint32_t>>(k, any_cast<vector<uint32_t>>(p)); } },
    { type_index(typeid(vector<uint64_t>)),                [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<uint64_t>>(k, any_cast<vector<uint64_t>>(p)); } },
    { type_index(typeid(vector<long>)),                    [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<long>>(k, any_cast<vector<long>>(p)); } },
    { type_index(typeid(vector<long long>)),               [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<long long>>(k, any_cast<vector<long long>>(p)); } },
    { type_index(typeid(vector<unsigned>)),                [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<unsigned>>(k, any_cast<vector<unsigned>>(p)); } },
    { type_index(typeid(vector<unsigned long>)),           [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<unsigned long>>(k, any_cast<vector<unsigned long>>(p)); } },
    { type_index(typeid(vector<unsigned long long>)),      [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<unsigned long long>>(k, any_cast<vector<unsigned long long>>(p)); } },
    { type_index(typeid(vector<float>)),                   [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<float>>(k, any_cast<vector<float>>(p)); } },
    { type_index(typeid(vector<double>)),                  [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<double>>(k, any_cast<vector<double>>(p)); } },
    { type_index(typeid(vector<long double>)),             [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<long double>>(k, any_cast<vector<long double>>(p)); } },
    { type_index(typeid(vector<boost::filesystem::path>)), [](const EventManager& em, const string& k, const Property& p) { em.Emit<PropertyChange, vector<boost::filesystem::path>>(k, any_cast<vector<boost::filesystem::path>>(p)); } },
};

} // namespace mq
} // namespace fair

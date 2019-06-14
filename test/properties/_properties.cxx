/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/ProgOptions.h>
#include <fairmq/EventManager.h>

#include <boost/filesystem.hpp>

#include <gtest/gtest.h>

#include <string>

namespace
{

using namespace std;

struct TestEvent : fair::mq::Event<const std::string&> {};

TEST(Properties, ConversionToString)
{
    fair::mq::ProgOptions o;
    o.SetProperty<char>("_char", 'a');
    o.SetProperty<signed char>("_signed char", -2);
    o.SetProperty<unsigned char>("_unsigned char", 2);
    o.SetProperty<const char*>("_const char*", "testcstring");
    o.SetProperty<string>("_string", "teststring");
    o.SetProperty<int>("_int", 1);
    o.SetProperty<size_t>("_size_t", 11);
    o.SetProperty<uint32_t>("_uint32_t", 12);
    o.SetProperty<uint64_t>("_uint64_t", 123);
    o.SetProperty<long>("_long", 1234);
    o.SetProperty<long long>("_long long", 12345);
    o.SetProperty<unsigned>("_unsigned", 3);
    o.SetProperty<unsigned long>("_unsigned long", 32);
    o.SetProperty<unsigned long long>("_unsigned long long", 321);
    o.SetProperty<float>("_float", 3.2);
    o.SetProperty<double>("_double", 33.22);
    o.SetProperty<long double>("_long double", 333.222);
    o.SetProperty<bool>("_bool", true);
    o.SetProperty<vector<bool>>("_vector<bool>", { true, true });
    o.SetProperty<boost::filesystem::path>("_boost::filesystem::path", boost::filesystem::path("C:\\Windows"));
    o.SetProperty<vector<char>>("_vector<char>", { 'a', 'b', 'c' });
    o.SetProperty<vector<signed char>>("_vector<signed char>", { -1, -2, -3 });
    o.SetProperty<vector<unsigned char>>("_vector<unsigned char>", { 1, 2, 3 });
    o.SetProperty<vector<string>>("_vector<string>", { "aa", "bb", "cc" });
    o.SetProperty<vector<int>>("_vector<int>", { 1, 2, 3 });
    o.SetProperty<vector<size_t>>("_vector<size_t>", { 1, 2, 3 });
    o.SetProperty<vector<uint32_t>>("_vector<uint32_t>", { 12, 13, 14 });
    o.SetProperty<vector<uint64_t>>("_vector<uint64_t>", { 123, 124, 125 });
    o.SetProperty<vector<long>>("_vector<long>", { 1234, 1235, 1236 });
    o.SetProperty<vector<long long>>("_vector<long long>", { 12345, 12346, 12347 });
    o.SetProperty<vector<unsigned>>("_vector<unsigned>", { 3, 4, 5 });
    o.SetProperty<vector<unsigned long>>("_vector<unsigned long>", { 32, 33, 34 });
    o.SetProperty<vector<unsigned long long>>("_vector<unsigned long long>", { 321, 322, 323 });
    o.SetProperty<vector<float>>("_vector<float>", { 3.2, 3.3, 3.4 });
    o.SetProperty<vector<double>>("_vector<double>", { 33.22, 33.23, 33.24 });
    o.SetProperty<vector<long double>>("_vector<long double>", { 333.222, 333.223, 333.224 });
    o.SetProperty<vector<boost::filesystem::path>>("_vector<boost::filesystem::path>", { boost::filesystem::path("C:\\Windows"), boost::filesystem::path("C:\\Windows\\System32") });

    ASSERT_EQ(o.GetPropertyAsString("_char"), "a");
    ASSERT_EQ(o.GetPropertyAsString("_signed char"), "-2");
    ASSERT_EQ(o.GetPropertyAsString("_unsigned char"), "2");
    ASSERT_EQ(o.GetPropertyAsString("_const char*"), "testcstring");
    ASSERT_EQ(o.GetPropertyAsString("_string"), "teststring");
    ASSERT_EQ(o.GetPropertyAsString("_int"), "1");
    ASSERT_EQ(o.GetPropertyAsString("_size_t"), "11");
    ASSERT_EQ(o.GetPropertyAsString("_uint32_t"), "12");
    ASSERT_EQ(o.GetPropertyAsString("_uint64_t"), "123");
    ASSERT_EQ(o.GetPropertyAsString("_long"), "1234");
    ASSERT_EQ(o.GetPropertyAsString("_long long"), "12345");
    ASSERT_EQ(o.GetPropertyAsString("_unsigned"), "3");
    ASSERT_EQ(o.GetPropertyAsString("_unsigned long"), "32");
    ASSERT_EQ(o.GetPropertyAsString("_unsigned long long"), "321");
    ASSERT_EQ(o.GetPropertyAsString("_float"), "3.2");
    ASSERT_EQ(o.GetPropertyAsString("_double"), "33.22");
    ASSERT_EQ(o.GetPropertyAsString("_long double"), "333.222");
    ASSERT_EQ(o.GetPropertyAsString("_bool"), "true");
    ASSERT_EQ(o.GetPropertyAsString("_vector<bool>"), "true, true");
    ASSERT_EQ(o.GetPropertyAsString("_boost::filesystem::path"), "\"C:\\Windows\"");
    ASSERT_EQ(o.GetPropertyAsString("_vector<char>"), "a, b, c");
    ASSERT_EQ(o.GetPropertyAsString("_vector<signed char>"), "-1, -2, -3");
    ASSERT_EQ(o.GetPropertyAsString("_vector<unsigned char>"), "1, 2, 3");
    ASSERT_EQ(o.GetPropertyAsString("_vector<string>"), "aa, bb, cc");
    ASSERT_EQ(o.GetPropertyAsString("_vector<int>"), "1, 2, 3");
    ASSERT_EQ(o.GetPropertyAsString("_vector<size_t>"), "1, 2, 3");
    ASSERT_EQ(o.GetPropertyAsString("_vector<uint32_t>"), "12, 13, 14");
    ASSERT_EQ(o.GetPropertyAsString("_vector<uint64_t>"), "123, 124, 125");
    ASSERT_EQ(o.GetPropertyAsString("_vector<long>"), "1234, 1235, 1236");
    ASSERT_EQ(o.GetPropertyAsString("_vector<long long>"), "12345, 12346, 12347");
    ASSERT_EQ(o.GetPropertyAsString("_vector<unsigned>"), "3, 4, 5");
    ASSERT_EQ(o.GetPropertyAsString("_vector<unsigned long>"), "32, 33, 34");
    ASSERT_EQ(o.GetPropertyAsString("_vector<unsigned long long>"), "321, 322, 323");
    ASSERT_EQ(o.GetPropertyAsString("_vector<float>"), "3.2, 3.3, 3.4");
    ASSERT_EQ(o.GetPropertyAsString("_vector<double>"), "33.22, 33.23, 33.24");
    ASSERT_EQ(o.GetPropertyAsString("_vector<long double>"), "333.222, 333.223, 333.224");
    ASSERT_EQ(o.GetPropertyAsString("_vector<boost::filesystem::path>"), "\"C:\\Windows\", \"C:\\Windows\\System32\"");
}

TEST(Properties, EventEmissionAndStringConversion)
{
    fair::mq::ProgOptions o;

    o.Subscribe<char>("test", [](const string& /* key */, char e) { ASSERT_EQ(e, 'a'); });
    o.Subscribe<signed char>("test", [](const string& /* key */, signed char e) { ASSERT_EQ(e, -2); });
    o.Subscribe<unsigned char>("test", [](const string& /* key */, unsigned char e) { ASSERT_EQ(e, 2); });
    o.Subscribe<const char*>("test", [](const string& /* key */, const char* e) { ASSERT_EQ(e, "testcstring"); });
    o.Subscribe<string>("test", [](const string& /* key */, string e) { ASSERT_EQ(e, "teststring"); });
    o.Subscribe<int>("test", [](const string& /* key */, int e) { ASSERT_EQ(e, 1); });
    o.Subscribe<long>("test", [](const string& /* key */, long e) { ASSERT_EQ(e, 1234); });
    o.Subscribe<long long>("test", [](const string& /* key */, long long e) { ASSERT_EQ(e, 12345); });
    o.Subscribe<unsigned>("test", [](const string& /* key */, unsigned e) { ASSERT_EQ(e, 3); });
    o.Subscribe<unsigned long>("test", [](const string& /* key */, unsigned long e) { ASSERT_EQ(e, 32); });
    o.Subscribe<unsigned long long>("test", [](const string& /* key */, unsigned long long e) { ASSERT_EQ(e, 321); });
    o.Subscribe<float>("test", [](const string& /* key */, float e) { ASSERT_EQ(e, 3.2f); });
    o.Subscribe<double>("test", [](const string& /* key */, double e) { ASSERT_EQ(e, 33.22); });
    o.Subscribe<long double>("test", [](const string& /* key */, long double e) { ASSERT_EQ(e, 333.222); });
    o.Subscribe<bool>("test", [](const string& /* key */, bool e) { ASSERT_EQ(e, true); });
    o.Subscribe<boost::filesystem::path>("test", [](const string& /* key */, boost::filesystem::path e) { ASSERT_EQ(e, boost::filesystem::path("C:\\Windows")); });
    o.Subscribe<vector<bool>>("test", [](const string& /* key */, vector<bool> e) { ASSERT_EQ(e, vector<bool>({ true, true })); });
    o.Subscribe<vector<char>>("test", [](const string& /* key */, vector<char> e) { ASSERT_EQ(e, vector<char>({ 'a', 'b', 'c' })); });
    o.Subscribe<vector<signed char>>("test", [](const string& /* key */, vector<signed char> e) { ASSERT_EQ(e, vector<signed char>({ -1, -2, -3 })); });
    o.Subscribe<vector<unsigned char>>("test", [](const string& /* key */, vector<unsigned char> e) { ASSERT_EQ(e, vector<unsigned char>({ 1, 2, 3 })); });
    o.Subscribe<vector<string>>("test", [](const string& /* key */, vector<string> e) { ASSERT_EQ(e, vector<string>({ "aa", "bb", "cc" })); });
    o.Subscribe<vector<int>>("test", [](const string& /* key */, vector<int> e) { ASSERT_EQ(e, vector<int>({ 1, 2, 3 })); });
    o.Subscribe<vector<long>>("test", [](const string& /* key */, vector<long> e) { ASSERT_EQ(e, vector<long>({ 1234, 1235, 1236 })); });
    o.Subscribe<vector<long long>>("test", [](const string& /* key */, vector<long long> e) { ASSERT_EQ(e, vector<long long>({ 12345, 12346, 12347 })); });
    o.Subscribe<vector<unsigned>>("test", [](const string& /* key */, vector<unsigned> e) { ASSERT_EQ(e, vector<unsigned>({ 3, 4, 5 })); });
    o.Subscribe<vector<unsigned long>>("test", [](const string& /* key */, vector<unsigned long> e) { ASSERT_EQ(e, vector<unsigned long>({ 32, 33, 34 })); });
    o.Subscribe<vector<unsigned long long>>("test", [](const string& /* key */, vector<unsigned long long> e) { ASSERT_EQ(e, vector<unsigned long long>({ 321, 322, 323 })); });
    o.Subscribe<vector<float>>("test", [](const string& /* key */, vector<float> e) { ASSERT_EQ(e, vector<float>({ 3.2, 3.3, 3.4 })); });
    o.Subscribe<vector<double>>("test", [](const string& /* key */, vector<double> e) { ASSERT_EQ(e, vector<double>({ 33.22, 33.23, 33.24 })); });
    o.Subscribe<vector<long double>>("test", [](const string& /* key */, vector<long double> e) { ASSERT_EQ(e, vector<long double>({ 333.222, 333.223, 333.224 })); });
    o.Subscribe<vector<boost::filesystem::path>>("test", [](const string& /* key */, vector<boost::filesystem::path> e) { ASSERT_EQ(e, vector<boost::filesystem::path>({ boost::filesystem::path("C:\\Windows"), boost::filesystem::path("C:\\Windows\\System32") })); });

    o.SetProperty<char>("_char", 'a');
    o.SetProperty<signed char>("_signed char", -2);
    o.SetProperty<unsigned char>("_unsigned char", 2);
    o.SetProperty<const char*>("_const char*", "testcstring");
    o.SetProperty<string>("_string", "teststring");
    o.SetProperty<int>("_int", 1);
    o.SetProperty<long>("_long", 1234);
    o.SetProperty<long long>("_long long", 12345);
    o.SetProperty<unsigned>("_unsigned", 3);
    o.SetProperty<unsigned long>("_unsigned long", 32);
    o.SetProperty<unsigned long long>("_unsigned long long", 321);
    o.SetProperty<float>("_float", 3.2);
    o.SetProperty<double>("_double", 33.22);
    o.SetProperty<long double>("_long double", 333.222);
    o.SetProperty<bool>("_bool", true);
    o.SetProperty<vector<bool>>("_vector<bool>", { true, true });
    o.SetProperty<boost::filesystem::path>("_boost::filesystem::path", boost::filesystem::path("C:\\Windows"));
    o.SetProperty<vector<char>>("_vector<char>", { 'a', 'b', 'c' });
    o.SetProperty<vector<signed char>>("_vector<signed char>", { -1, -2, -3 });
    o.SetProperty<vector<unsigned char>>("_vector<unsigned char>", { 1, 2, 3 });
    o.SetProperty<vector<string>>("_vector<string>", { "aa", "bb", "cc" });
    o.SetProperty<vector<int>>("_vector<int>", { 1, 2, 3 });
    o.SetProperty<vector<long>>("_vector<long>", { 1234, 1235, 1236 });
    o.SetProperty<vector<long long>>("_vector<long long>", { 12345, 12346, 12347 });
    o.SetProperty<vector<unsigned>>("_vector<unsigned>", { 3, 4, 5 });
    o.SetProperty<vector<unsigned long>>("_vector<unsigned long>", { 32, 33, 34 });
    o.SetProperty<vector<unsigned long long>>("_vector<unsigned long long>", { 321, 322, 323 });
    o.SetProperty<vector<float>>("_vector<float>", { 3.2, 3.3, 3.4 });
    o.SetProperty<vector<double>>("_vector<double>", { 33.22, 33.23, 33.24 });
    o.SetProperty<vector<long double>>("_vector<long double>", { 333.222, 333.223, 333.224 });
    o.SetProperty<vector<boost::filesystem::path>>("_vector<boost::filesystem::path>", { boost::filesystem::path("C:\\Windows"), boost::filesystem::path("C:\\Windows\\System32") });
}

} // namespace

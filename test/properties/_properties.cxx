/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/ProgOptions.h>

#include <boost/filesystem.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

namespace
{

using namespace std;
using namespace fair::mq;

template<typename T>
auto set_and_get(ProgOptions& options, const string& key, T value) -> void
{
    std::cout << key << std::endl;
    options.SetProperty(key, value);
    EXPECT_EQ(options.GetPropertyAsString(key),
              PropertyHelper::ConvertPropertyToString(Property(value)));
    EXPECT_EQ(options.GetProperty<T>(key), value);
}

TEST(ProgOptions, SetAndGet)
{
    ProgOptions o;

    set_and_get<char>(o, "_char", 'a');
    set_and_get<signed char>(o, "_signed char", -2);
    set_and_get<unsigned char>(o, "_unsigned char", 2);
    set_and_get<const char*>(o, "_const char*", "testcstring");
    set_and_get<string>(o, "_string", "teststring");
    set_and_get<int>(o, "_int", 1);
    set_and_get<size_t>(o, "_size_t", 11);
    set_and_get<uint32_t>(o, "_uint32_t", 12);
    set_and_get<uint64_t>(o, "_uint64_t", 123);
    set_and_get<long>(o, "_long", 1234);
    set_and_get<long long>(o, "_long long", 12345);
    set_and_get<unsigned>(o, "_unsigned", 3);
    set_and_get<unsigned long>(o, "_unsigned long", 32);
    set_and_get<unsigned long long>(o, "_unsigned long long", 321);
    set_and_get<float>(o, "_float", 3.2);
    set_and_get<double>(o, "_double", 33.22);
    set_and_get<long double>(o, "_long double", 333.222);
    set_and_get<bool>(o, "_bool", true);
    set_and_get<boost::filesystem::path>(o, "_boost::filesystem::path", boost::filesystem::path("C:\\Windows"));
    set_and_get<vector<bool>>(o, "_vector<bool>", { true, true });
    set_and_get<vector<char>>(o, "_vector<char>", { 'a', 'b', 'c' });
    set_and_get<vector<signed char>>(o, "_vector<signed char>", { -1, -2, -3 });
    set_and_get<vector<unsigned char>>(o, "_vector<unsigned char>", { 1, 2, 3 });
    set_and_get<vector<string>>(o, "_vector<string>", { "aa", "bb", "cc" });
    set_and_get<vector<int>>(o, "_vector<int>", { 1, 2, 3 });
    set_and_get<vector<size_t>>(o, "_vector<size_t>", { 1, 2, 3 });
    set_and_get<vector<uint32_t>>(o, "_vector<uint32_t>", { 12, 13, 14 });
    set_and_get<vector<uint64_t>>(o, "_vector<uint64_t>", { 123, 124, 125 });
    set_and_get<vector<long>>(o, "_vector<long>", { 1234, 1235, 1236 });
    set_and_get<vector<long long>>(o, "_vector<long long>", { 12345, 12346, 12347 });
    set_and_get<vector<unsigned>>(o, "_vector<unsigned>", { 3, 4, 5 });
    set_and_get<vector<unsigned long>>(o, "_vector<unsigned long>", { 32, 33, 34 });
    set_and_get<vector<unsigned long long>>(o, "_vector<unsigned long long>", { 321, 322, 323 });
    set_and_get<vector<float>>(o, "_vector<float>", { 3.2, 3.3, 3.4 });
    set_and_get<vector<double>>(o, "_vector<double>", { 33.22, 33.23, 33.24 });
    set_and_get<vector<long double>>(o, "_vector<long double>", { 333.222, 333.223, 333.224 });
    set_and_get<vector<boost::filesystem::path>>(o, "_vector<boost::filesystem::path>", { boost::filesystem::path("C:\\Windows"), boost::filesystem::path("C:\\Windows\\System32") });

    ASSERT_THROW(o.ParseAll({"cmd", "--unregistered", "option"}, false), boost::program_options::unknown_option);
    ASSERT_NO_THROW(o.ParseAll({"cmd", "--unregistered", "option"}, true));
}

template<typename T>
auto subscribe_and_set(ProgOptions& options, const string& expected_key, T expected_value)
    -> void
{
    std::cout << expected_key << std::endl;
    const string subscriber("test");
    const string different_key("different");
    ASSERT_NE(expected_key, different_key);
    int type_events(0);
    int asstring_events(0);
    int total_events(0);
    options.Subscribe<T>(subscriber, [&](const string& key, T value) {
        if (expected_key == key) {
            ++type_events;
            EXPECT_EQ(value, expected_value);
        }
        ++total_events;
    });
    options.SubscribeAsString(subscriber, [&](const string& key, string value) {
        if (expected_key == key) {
            ++asstring_events;
            EXPECT_EQ(value, PropertyHelper::ConvertPropertyToString(Property(value)));
        }
        ++total_events;
    });
    options.SetProperty<T>(expected_key, expected_value);
    options.Unsubscribe<T>(subscriber);
    options.SetProperty<T>(different_key, expected_value);
    options.SetProperty<T>(expected_key, expected_value);
    options.UnsubscribeAsString(subscriber);
    options.SetProperty<T>(expected_key, expected_value);
    EXPECT_EQ(type_events, 1);
    EXPECT_EQ(asstring_events, 2);
    EXPECT_EQ(total_events, 4);
}

TEST(ProgOptions, SubscribeAndSet)
{
    ProgOptions o;

    subscribe_and_set<char>(o, "_char", 'a');
    subscribe_and_set<signed char>(o, "_signed char", -2);
    subscribe_and_set<unsigned char>(o, "_unsigned char", 2);
    subscribe_and_set<const char*>(o, "_const char*", "testcstring");
    subscribe_and_set<string>(o, "_string", "teststring");
    subscribe_and_set<int>(o, "_int", 1);
    subscribe_and_set<size_t>(o, "_size_t", 11);
    subscribe_and_set<uint32_t>(o, "_uint32_t", 12);
    subscribe_and_set<uint64_t>(o, "_uint64_t", 123);
    subscribe_and_set<long>(o, "_long", 1234);
    subscribe_and_set<long long>(o, "_long long", 12345);
    subscribe_and_set<unsigned>(o, "_unsigned", 3);
    subscribe_and_set<unsigned long>(o, "_unsigned long", 32);
    subscribe_and_set<unsigned long long>(o, "_unsigned long long", 321);
    subscribe_and_set<float>(o, "_float", 3.2);
    subscribe_and_set<double>(o, "_double", 33.22);
    subscribe_and_set<long double>(o, "_long double", 333.222);
    subscribe_and_set<bool>(o, "_bool", true);
    subscribe_and_set<boost::filesystem::path>(o, "_boost::filesystem::path", boost::filesystem::path("C:\\Windows"));
    subscribe_and_set<vector<bool>>(o, "_vector<bool>", { true, true });
    subscribe_and_set<vector<char>>(o, "_vector<char>", { 'a', 'b', 'c' });
    subscribe_and_set<vector<signed char>>(o, "_vector<signed char>", { -1, -2, -3 });
    subscribe_and_set<vector<unsigned char>>(o, "_vector<unsigned char>", { 1, 2, 3 });
    subscribe_and_set<vector<string>>(o, "_vector<string>", { "aa", "bb", "cc" });
    subscribe_and_set<vector<int>>(o, "_vector<int>", { 1, 2, 3 });
    subscribe_and_set<vector<size_t>>(o, "_vector<size_t>", { 1, 2, 3 });
    subscribe_and_set<vector<uint32_t>>(o, "_vector<uint32_t>", { 12, 13, 14 });
    subscribe_and_set<vector<uint64_t>>(o, "_vector<uint64_t>", { 123, 124, 125 });
    subscribe_and_set<vector<long>>(o, "_vector<long>", { 1234, 1235, 1236 });
    subscribe_and_set<vector<long long>>(o, "_vector<long long>", { 12345, 12346, 12347 });
    subscribe_and_set<vector<unsigned>>(o, "_vector<unsigned>", { 3, 4, 5 });
    subscribe_and_set<vector<unsigned long>>(o, "_vector<unsigned long>", { 32, 33, 34 });
    subscribe_and_set<vector<unsigned long long>>(o, "_vector<unsigned long long>", { 321, 322, 323 });
    subscribe_and_set<vector<float>>(o, "_vector<float>", { 3.2, 3.3, 3.4 });
    subscribe_and_set<vector<double>>(o, "_vector<double>", { 33.22, 33.23, 33.24 });
    subscribe_and_set<vector<long double>>(o, "_vector<long double>", { 333.222, 333.223, 333.224 });
    subscribe_and_set<vector<boost::filesystem::path>>(o, "_vector<boost::filesystem::path>", { boost::filesystem::path("C:\\Windows"), boost::filesystem::path("C:\\Windows\\System32") });
}

TEST(PropertyHelper, ConvertPropertyToString)
{
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<char>('a'))), "a");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<signed char>(-2))), "-2");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<unsigned char>(2))), "2");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<const char*>("testcstring"))), "testcstring");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<string>("teststring"))), "teststring");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<int>(1))), "1");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<size_t>(11))), "11");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<uint32_t>(12))), "12");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<uint64_t>(123))), "123");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<long>(1234))), "1234");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<long long>(12345))), "12345");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<unsigned>(3))), "3");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<unsigned long>(32))), "32");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<unsigned long long>(321))), "321");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<float>(3.2))), "3.2");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<double>(33.22))), "33.22");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<long double>(333.222))), "333.222");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(static_cast<bool>(true))), "true");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(boost::filesystem::path("C:\\Windows"))), "\"C:\\Windows\"");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<char>({ 'a', 'b', 'c' }))), "a, b, c");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<signed char>({ -1, -2, -3 }))), "-1, -2, -3");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<unsigned char>({ 1, 2, 3 }))), "1, 2, 3");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<string>({ "aa", "bb", "cc" }))), "aa, bb, cc");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<int>({ 1, 2, 3 }))), "1, 2, 3");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<size_t>({ 1, 2, 3 }))), "1, 2, 3");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<uint32_t>({ 12, 13, 14 }))), "12, 13, 14");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<uint64_t>({ 123, 124, 125 }))), "123, 124, 125");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<long>({ 1234, 1235, 1236 }))), "1234, 1235, 1236");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<long long>({ 12345, 12346, 12347 }))), "12345, 12346, 12347");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<unsigned>({ 3, 4, 5 }))), "3, 4, 5");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<unsigned long>({ 32, 33, 34 }))), "32, 33, 34");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<unsigned long long>({ 321, 322, 323 }))), "321, 322, 323");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<float>({ 3.2, 3.3, 3.4 }))), "3.2, 3.3, 3.4");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<double>({ 33.22, 33.23, 33.24 }))), "33.22, 33.23, 33.24");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<long double>({ 333.222, 333.223, 333.224 }))), "333.222, 333.223, 333.224");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<bool>({ true, true }))), "true, true");
    EXPECT_EQ(PropertyHelper::ConvertPropertyToString(Property(vector<boost::filesystem::path>({ boost::filesystem::path("C:\\Windows"), boost::filesystem::path("C:\\Windows\\System32") }))), "\"C:\\Windows\", \"C:\\Windows\\System32\"");
}

} // namespace

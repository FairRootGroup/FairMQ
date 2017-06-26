/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <gtest/gtest.h>
#include <fairmq/PluginManager.h>
#include <fairmq/PluginServices.h>
#include <FairMQDevice.h>
#include <options/FairMQProgOptions.h>
#include <FairMQLogger.h>
#include <fstream>
#include <vector>

namespace
{

using namespace fair::mq;
using namespace boost::filesystem;
using namespace boost::program_options;
using namespace std;

auto control(FairMQDevice& device) -> void
{
    device.SetTransport("zeromq");
    for (const auto event : {
        FairMQDevice::INIT_DEVICE,
        FairMQDevice::RESET_DEVICE,
        FairMQDevice::END,
    }) {
        device.ChangeState(event);
        if (event != FairMQDevice::END) device.WaitForEndOfState(event);
    }
}

TEST(PluginManager, LoadPlugin)
{
    auto config = FairMQProgOptions{};
    FairMQDevice device{};
    auto mgr = PluginManager{};
    mgr.EmplacePluginServices(config, device);

    mgr.PrependSearchPath("./lib");

    ASSERT_NO_THROW(mgr.LoadPlugin("test_dummy"));
    ASSERT_NO_THROW(mgr.LoadPlugin("test_dummy2"));

    ASSERT_NO_THROW(mgr.InstantiatePlugins());

    // check order
    const auto expected = vector<string>{"test_dummy", "test_dummy2"};
    auto actual = vector<string>{};
    mgr.ForEachPlugin([&](Plugin& plugin){ actual.push_back(plugin.GetName()); });
    ASSERT_TRUE(actual == expected);

    // program options
    auto count = 0;
    mgr.ForEachPluginProgOptions([&count](const options_description& d){ ++count; });
    ASSERT_EQ(count, 1);

    control(device);
}

TEST(PluginManager, Factory)
{
    const auto args = vector<string>{"-l", "debug", "--help", "-S", ">/lib", "</home/user/lib", "/usr/local/lib", "/usr/lib"};
    auto mgr = PluginManager::MakeFromCommandLineOptions(args);
    const auto path1 = path{"/home/user/lib"};
    const auto path2 = path{"/usr/local/lib"};
    const auto path3 = path{"/usr/lib"};
    const auto path4 = path{"/lib"};
    const auto expected = vector<path>{path1, path2, path3, path4};
    ASSERT_TRUE(static_cast<bool>(mgr));
    ASSERT_TRUE(mgr->SearchPaths() == expected);
}

TEST(PluginManager, SearchPathValidation)
{
    const auto path1 = path{"/tmp/test1"};
    const auto path2 = path{"/tmp/test2"};
    const auto path3 = path{"/tmp/test3"};
    auto mgr = PluginManager{};

    mgr.SetSearchPaths({path1, path2});
    auto expected = vector<path>{path1, path2};
    ASSERT_EQ(mgr.SearchPaths(), expected);

    mgr.AppendSearchPath(path3);
    expected = vector<path>{path1, path2, path3};
    ASSERT_EQ(mgr.SearchPaths(), expected);

    mgr.PrependSearchPath(path3);
    expected = vector<path>{path3, path1, path2, path3};
    ASSERT_EQ(mgr.SearchPaths(), expected);
}

TEST(PluginManager, SearchPaths)
{
    const auto temp = temp_directory_path() / unique_path();
    create_directories(temp);
    const auto non_existing_dir = temp / "non-existing-dir";
    const auto existing_dir = temp / "existing-dir";
    create_directories(existing_dir);
    const auto existing_file = temp / "existing-file.so";
    std::fstream fs;
    fs.open(existing_file.string(), std::fstream::out);
    fs.close();
    const auto empty_path = path{""};

    auto mgr = PluginManager{};
    ASSERT_NO_THROW(mgr.AppendSearchPath(non_existing_dir));
    ASSERT_NO_THROW(mgr.AppendSearchPath(existing_dir));
    ASSERT_THROW(mgr.AppendSearchPath(existing_file), PluginManager::BadSearchPath);
    ASSERT_NO_THROW(mgr.PrependSearchPath(non_existing_dir));
    ASSERT_NO_THROW(mgr.PrependSearchPath(existing_dir));
    ASSERT_THROW(mgr.PrependSearchPath(existing_file), PluginManager::BadSearchPath);
    ASSERT_NO_THROW(mgr.SetSearchPaths({non_existing_dir, existing_dir}));
    ASSERT_THROW(mgr.SetSearchPaths({non_existing_dir, existing_file}), PluginManager::BadSearchPath);
    ASSERT_THROW(mgr.SetSearchPaths({non_existing_dir, empty_path}), PluginManager::BadSearchPath);

    remove_all(temp);
}

} /* namespace */

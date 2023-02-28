/********************************************************************************
 * Copyright (C) 2017-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "../helper/ControlDevice.h"

#include <fairmq/PluginManager.h>
#include <fairmq/PluginServices.h>
#include <fairmq/Device.h>
#include <fairmq/ProgOptions.h>

#include <gtest/gtest.h>

#include <fairlogger/Logger.h>

#include <fstream>
#include <memory>
#include <vector>
#include <thread>

namespace _plugin_manager
{

using namespace fair::mq;
using namespace boost::filesystem;
using namespace boost::program_options;
using namespace std;

auto control(Device& device) -> void
{
    device.SetTransport("zeromq");
    test::Control(device, test::Cycle::ToDeviceReadyAndBack);
}

TEST(PluginManager, LoadPluginDynamic)
{
    ProgOptions config;
    Device device;
    PluginManager mgr;
    mgr.EmplacePluginServices(config, device);

    mgr.PrependSearchPath("./test");

    ASSERT_NO_THROW(mgr.LoadPlugin("test_dummy"));
    ASSERT_NO_THROW(mgr.LoadPlugin("test_dummy2"));

    ASSERT_NO_THROW(mgr.InstantiatePlugins());

    // check order
    const auto expected = vector<string>{"test_dummy", "test_dummy2"};
    auto actual = vector<string>();
    mgr.ForEachPlugin([&](Plugin& plugin){ actual.push_back(plugin.GetName()); });
    ASSERT_TRUE(actual == expected);

    // program options
    auto count = 0;
    mgr.ForEachPluginProgOptions([&count](const options_description& /*d*/){ ++count; });
    ASSERT_EQ(count, 1);

    thread t(control, std::ref(device));
    device.RunStateMachine();
    if (t.joinable()) {
        t.join();
    }
}

TEST(PluginManager, LoadPluginStatic)
{
    Device device;
    PluginManager mgr;
    device.SetTransport("zeromq");

    ASSERT_NO_THROW(mgr.LoadPlugin("s:control"));

    ProgOptions config;
    config.SetProperty<string>("control", "static");
    config.SetProperty("catch-signals", 0);
    mgr.EmplacePluginServices(config, device);

    ASSERT_NO_THROW(mgr.InstantiatePlugins());

    // check order
    const auto expected = vector<string>{"control"};
    auto actual = vector<string>();
    mgr.ForEachPlugin([&](Plugin& plugin){ actual.push_back(plugin.GetName()); });
    ASSERT_TRUE(actual == expected);

    // program options
    auto count = 0;
    mgr.ForEachPluginProgOptions([&count](const options_description&){ ++count; });
    ASSERT_EQ(count, 1);

    device.RunStateMachine();

    mgr.WaitForPluginsToReleaseDeviceControl();
}

TEST(PluginManager, Factory)
{
    const auto args = vector<string>{"-l", "debug", "--help", "-S", ">/lib", "</home/user/lib", "/usr/local/lib", "/usr/lib"};
    PluginManager mgr(args);
    const auto path1 = path{"/home/user/lib"};
    const auto path2 = path{"/usr/local/lib"};
    const auto path3 = path{"/usr/lib"};
    const auto path4 = path{"/lib"};
    const auto expected = vector<path>{path1, path2, path3, path4};
    // ASSERT_TRUE(static_cast<bool>(mgr));
    ASSERT_TRUE(mgr.SearchPaths() == expected);
}

TEST(PluginManager, SearchPathValidation)
{
    const auto path1 = path{"/tmp/test1"};
    const auto path2 = path{"/tmp/test2"};
    const auto path3 = path{"/tmp/test3"};
    PluginManager mgr;

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

    PluginManager mgr;
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

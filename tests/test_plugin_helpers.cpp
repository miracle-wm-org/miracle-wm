/**
Copyright (C) 2025  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include "plugin_helpers.h"

#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace miracle;
using namespace testing;

namespace
{
std::string make_temp_plugin(std::string const& name, std::string const& contents)
{
    auto path = (std::filesystem::current_path() / name).string();
    std::ofstream f(path, std::ios::binary);
    f << contents;
    return path;
}

PluginLoadResult make_success(PluginHandle handle)
{
    return PluginLoadResult { .success = true, .handle = handle, .error = "" };
}

PluginLoadResult make_failure()
{
    return PluginLoadResult { .success = false, .handle = 0, .error = "Unable to lo" };
}
}

class TestPluginHelpers : public testing::Test
{
public:
    void TearDown() override
    {
        for (auto const& p : created_files)
            std::filesystem::remove(p);
    }

    std::string make_plugin(std::string const& name, std::string const& contents = "plugin-bytes")
    {
        auto path = make_temp_plugin(name, contents);
        created_files.push_back(path);
        return path;
    }

    std::vector<std::string> created_files;
    std::map<std::string, LoadedPlugin> loaded;
    std::vector<PluginHandle> unloaded_handles;
    PluginHandle next_handle = 1;

    auto make_load()
    {
        return [this](std::string const&, std::string const&) -> PluginLoadResult
        {
            return make_success(next_handle++);
        };
    }

    auto make_failing_load()
    {
        return [](std::string const&, std::string const&) -> PluginLoadResult
        {
            return make_failure();
        };
    }

    auto make_unload()
    {
        return [this](PluginHandle handle)
        {
            unloaded_handles.push_back(handle);
        };
    }
};

TEST_F(TestPluginHelpers, NewPluginIsLoaded)
{
    auto path = make_plugin("new_plugin.wasm");
    std::vector<PluginConfiguration> plugins {
        { path, "" }
    };

    update_plugins(plugins, loaded, make_load(), make_unload());

    EXPECT_EQ(loaded.size(), 1u);
    EXPECT_TRUE(loaded.count(path));
    EXPECT_TRUE(unloaded_handles.empty());
}

TEST_F(TestPluginHelpers, PluginRemovedFromConfigIsUnloaded)
{
    auto path = make_plugin("removed_plugin.wasm");
    loaded[path] = { 42u, "", hash_plugin_file(path) };

    update_plugins({}, loaded, make_load(), make_unload());

    EXPECT_TRUE(loaded.empty());
    ASSERT_EQ(unloaded_handles.size(), 1u);
    EXPECT_EQ(unloaded_handles[0], 42u);
}

TEST_F(TestPluginHelpers, UnchangedPluginIsNotReloaded)
{
    auto path = make_plugin("unchanged_plugin.wasm");
    auto const hash = hash_plugin_file(path);
    loaded[path] = { 7u, "", hash };

    std::vector<PluginConfiguration> plugins {
        { path, "" }
    };
    int load_calls = 0;
    auto counting_load = [&](std::string const&, std::string const&) -> PluginLoadResult
    {
        ++load_calls;
        return make_success(99u);
    };

    update_plugins(plugins, loaded, counting_load, make_unload());

    EXPECT_EQ(load_calls, 0);
    EXPECT_TRUE(unloaded_handles.empty());
    ASSERT_EQ(loaded.size(), 1u);
    EXPECT_EQ(loaded.at(path).handle, 7u);
}

TEST_F(TestPluginHelpers, ChangedUserdataTriggersReload)
{
    auto path = make_plugin("userdata_plugin.wasm");
    loaded[path] = { 1u, R"({"old":true})", hash_plugin_file(path) };

    std::vector<PluginConfiguration> plugins {
        { path, R"({"new":true})" }
    };

    update_plugins(plugins, loaded, make_load(), make_unload());

    ASSERT_EQ(unloaded_handles.size(), 1u);
    EXPECT_EQ(unloaded_handles[0], 1u);
    ASSERT_EQ(loaded.size(), 1u);
    EXPECT_EQ(loaded.at(path).userdata_json, R"({"new":true})");
}

TEST_F(TestPluginHelpers, ChangedFileHashTriggersReload)
{
    auto path = make_plugin("hash_plugin.wasm", "version-1");
    loaded[path] = { 5u, "", hash_plugin_file(path) };

    // Overwrite file with new contents so the hash changes
    {
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        f << "version-2";
    }

    std::vector<PluginConfiguration> plugins {
        { path, "" }
    };

    update_plugins(plugins, loaded, make_load(), make_unload());

    ASSERT_EQ(unloaded_handles.size(), 1u);
    EXPECT_EQ(unloaded_handles[0], 5u);
    EXPECT_EQ(loaded.size(), 1u);
}

TEST_F(TestPluginHelpers, FailedLoadDoesNotAddToLoadedPlugins)
{
    auto path = make_plugin("failing_plugin.wasm");
    std::vector<PluginConfiguration> plugins {
        { path, "" }
    };

    update_plugins(plugins, loaded, make_failing_load(), make_unload());

    EXPECT_TRUE(loaded.empty());
    EXPECT_TRUE(unloaded_handles.empty());
}

TEST_F(TestPluginHelpers, HashFileReturnsZeroForMissingFile)
{
    EXPECT_EQ(hash_plugin_file("/nonexistent/path/plugin.wasm"), 0u);
}

TEST_F(TestPluginHelpers, HashFileReturnsDifferentHashesForDifferentContents)
{
    auto path_a = make_plugin("hash_a.wasm", "contents-a");
    auto path_b = make_plugin("hash_b.wasm", "contents-b");
    EXPECT_NE(hash_plugin_file(path_a), hash_plugin_file(path_b));
}

TEST_F(TestPluginHelpers, HashFileReturnsSameHashForSameContents)
{
    auto path_a = make_plugin("same_a.wasm", "same-contents");
    auto path_b = make_plugin("same_b.wasm", "same-contents");
    EXPECT_EQ(hash_plugin_file(path_a), hash_plugin_file(path_b));
}

TEST_F(TestPluginHelpers, MultiplePluginsLoadedIndependently)
{
    auto path_a = make_plugin("multi_a.wasm", "a");
    auto path_b = make_plugin("multi_b.wasm", "b");
    std::vector<PluginConfiguration> plugins {
        { path_a, "" },
        { path_b, "" }
    };

    update_plugins(plugins, loaded, make_load(), make_unload());

    EXPECT_EQ(loaded.size(), 2u);
    EXPECT_TRUE(loaded.count(path_a));
    EXPECT_TRUE(loaded.count(path_b));
}

TEST_F(TestPluginHelpers, LoadedPluginCachesFileHash)
{
    auto path = make_plugin("cache_hash.wasm", "data");
    auto const expected_hash = hash_plugin_file(path);
    std::vector<PluginConfiguration> plugins {
        { path, "" }
    };

    update_plugins(plugins, loaded, make_load(), make_unload());

    ASSERT_TRUE(loaded.count(path));
    EXPECT_EQ(loaded.at(path).file_hash, expected_hash);
}

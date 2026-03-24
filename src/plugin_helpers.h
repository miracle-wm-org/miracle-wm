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

#ifndef MIRACLEWM_PLUGIN_HELPERS_H
#define MIRACLEWM_PLUGIN_HELPERS_H

#include "plugin_handle.h"
#include "plugin_manager.h"
#include <miracle/cpp/config-cpp.h>

#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace miracle
{
/// Represents a plugin that has been successfully loaded.
struct LoadedPlugin
{
    PluginHandle handle;
    std::string userdata_json;
    size_t file_hash;
};

/// Returns a hash of the file at \p path, or 0 if the file cannot be opened.
inline size_t hash_plugin_file(std::string const& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return 0;
    std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return std::hash<std::string> {}(contents);
}

/// Synchronises \p loaded_plugins with the desired \p plugins configuration.
///
/// A loaded plugin is unloaded (via \p unload) and removed from \p loaded_plugins when:
///   - It is no longer present in \p plugins, or
///   - Its userdata_json has changed, or
///   - The hash of its file on disk has changed.
///
/// A plugin that is present in \p plugins but not yet in \p loaded_plugins is loaded
/// via \p load and, on success, added to \p loaded_plugins.
inline void update_plugins(
    std::vector<PluginConfiguration> const& plugins,
    std::map<std::string, LoadedPlugin>& loaded_plugins,
    std::function<PluginLoadResult(std::string const&, std::string const&)> const& load,
    std::function<void(PluginHandle)> const& unload)
{
    struct PluginEntry
    {
        std::string userdata_json;
        size_t file_hash;
    };
    std::map<std::string, PluginEntry> new_plugin_map;
    for (auto const& p : plugins)
        new_plugin_map[p.path] = { p.userdata_json, hash_plugin_file(p.path) };

    for (auto it = loaded_plugins.begin(); it != loaded_plugins.end();)
    {
        auto const& [path, loaded] = *it;
        auto const found = new_plugin_map.find(path);
        if (found == new_plugin_map.end()
            || found->second.userdata_json != loaded.userdata_json
            || found->second.file_hash != loaded.file_hash)
        {
            unload(loaded.handle);
            it = loaded_plugins.erase(it);
        }
        else
            ++it;
    }

    for (auto const& p : plugins)
    {
        if (loaded_plugins.count(p.path) == 0)
        {
            auto const file_hash = new_plugin_map.at(p.path).file_hash;
            auto result = load(p.path, p.userdata_json);
            if (result.success)
                loaded_plugins[p.path] = { result.handle, p.userdata_json, file_hash };
        }
    }
}
}

#endif // MIRACLEWM_PLUGIN_HELPERS_H

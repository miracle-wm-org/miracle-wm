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

#ifndef MIRACLE_WM_SAMPLER_REGISTRY_H
#define MIRACLE_WM_SAMPLER_REGISTRY_H

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace miracle
{

struct SamplerRegistry
{
    struct Entry
    {
        uint8_t id;
        std::vector<std::string> passes;
        /// The plugin that registered this shader, if any. Shaders registered
        /// without an owner (e.g. from tests or built-in code) have no handle
        /// and are never removed by a plugin unload.
        std::optional<uint32_t> plugin_handle;
    };

    // TODO: This relies on non-local knowledge that Mir internally never
    //  claims identifiers above 1.
    uint8_t id = 5;
    std::vector<Entry> entries;
    std::mutex mutex;

    /// The single, global full-screen (output) shader, if any. Unlike window
    /// shaders this is not addressed by id: only one can be active at a time,
    /// and it overrides the config `output_filter.shader_path`.
    std::optional<std::string> screen_shader_source;
    std::optional<uint32_t> screen_shader_plugin_handle;
    /// Bumped on every change so the renderer can detect (and recompile on)
    /// transitions without comparing source strings.
    uint64_t screen_shader_generation = 0;

    uint8_t register_window_shader(
        std::vector<std::string> passes,
        std::optional<uint32_t> plugin_handle = std::nullopt);

    /// Remove all shaders registered by the given plugin and return their ids.
    /// Also clears the screen shader if the given plugin owns it.
    std::vector<uint8_t> remove_shaders_for_plugin(uint32_t plugin_handle);

    /// Set (or, with std::nullopt, clear) the global full-screen shader.
    void set_screen_shader(
        std::optional<std::string> source,
        std::optional<uint32_t> plugin_handle = std::nullopt);

    struct ScreenShaderState
    {
        std::optional<std::string> source;
        uint64_t generation;
    };

    /// A locked snapshot of the current screen shader, for the renderer.
    ScreenShaderState screen_shader_state();
};

} // miracle

#endif // MIRACLE_WM_SAMPLER_REGISTRY_H

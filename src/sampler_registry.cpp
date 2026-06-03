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

#include "sampler_registry.h"

#include <algorithm>

uint8_t miracle::SamplerRegistry::register_window_shader(
    std::vector<std::string> passes,
    std::optional<uint32_t> plugin_handle)
{
    std::lock_guard lock { mutex };
    auto const next_id = id++;
    entries.push_back(Entry { next_id, std::move(passes), plugin_handle });
    return next_id;
}

std::vector<uint8_t> miracle::SamplerRegistry::remove_shaders_for_plugin(uint32_t plugin_handle)
{
    std::lock_guard lock { mutex };
    std::vector<uint8_t> removed;
    auto const new_end = std::remove_if(entries.begin(), entries.end(), [&](Entry const& entry)
    {
        if (entry.plugin_handle == plugin_handle)
        {
            removed.push_back(entry.id);
            return true;
        }
        return false;
    });
    entries.erase(new_end, entries.end());

    if (screen_shader_plugin_handle == plugin_handle)
    {
        screen_shader_source = std::nullopt;
        screen_shader_plugin_handle = std::nullopt;
        ++screen_shader_generation;
    }

    return removed;
}

void miracle::SamplerRegistry::set_screen_shader(
    std::optional<std::string> source,
    std::optional<uint32_t> plugin_handle)
{
    std::lock_guard lock { mutex };
    screen_shader_source = std::move(source);
    screen_shader_plugin_handle = plugin_handle;
    ++screen_shader_generation;
}

miracle::SamplerRegistry::ScreenShaderState miracle::SamplerRegistry::screen_shader_state()
{
    std::lock_guard lock { mutex };
    return { screen_shader_source, screen_shader_generation };
}

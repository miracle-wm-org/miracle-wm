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

#include "render_data_manager.h"
#include "container.h"
#include <algorithm>
#include <mir/scene/surface.h>

using namespace miracle;

RenderDataManager::RenderDataManager()
{
    render_data.reserve(48);
}

RenderDataManagerId RenderDataManager::add(RenderData const&& data)
{
    std::lock_guard lock(mutex);
    auto id = next_id++;
    render_data.push_back(data);
    render_data.back().id = id;
    return id;
}

void RenderDataManager::transform_change(RenderDataManagerId id, glm::mat4 const& transform)
{
    std::lock_guard lock(mutex);
    for (auto& data : render_data)
    {
        if (data.id == id)
        {
            data.transform = transform;
            return;
        }
    }
}

void RenderDataManager::workspace_transform_change(RenderDataManagerId id, glm::mat4 const& transform)
{
    std::lock_guard lock(mutex);
    for (auto& data : render_data)
    {
        if (data.id == id)
        {
            data.workspace_transform = transform;
            return;
        }
    }
}

void RenderDataManager::output_area_change(RenderDataManagerId id, mir::geometry::Rectangle const& area)
{
    std::lock_guard lock(mutex);
    for (auto& data : render_data)
    {
        if (data.id == id)
        {
            data.output_area = area;
            return;
        }
    }
}

void RenderDataManager::focus_change(RenderDataManagerId id, bool is_focused)
{
    std::lock_guard lock(mutex);
    for (auto& data : render_data)
    {
        if (data.id == id)
        {
            data.is_focused = is_focused;
            return;
        }
    }
}

void RenderDataManager::needs_outline_change(RenderDataManagerId id, bool needs_outline)
{
    std::lock_guard lock(mutex);
    for (auto& data : render_data)
    {
        if (data.id == id)
        {
            data.needs_outline = needs_outline;
            return;
        }
    }
}

void RenderDataManager::shader_id_change(RenderDataManagerId id, std::optional<uint8_t> shader_id)
{
    std::lock_guard lock(mutex);
    for (auto& data : render_data)
    {
        if (data.id == id)
        {
            data.shader_id = shader_id;
            return;
        }
    }
}

void RenderDataManager::geometry_shader_id_change(RenderDataManagerId id, std::optional<uint8_t> geometry_shader_id)
{
    std::lock_guard lock(mutex);
    for (auto& data : render_data)
    {
        if (data.id == id)
        {
            data.geometry_shader_id = geometry_shader_id;
            return;
        }
    }
}

void RenderDataManager::reset_shaders(std::vector<uint8_t> const& ids)
{
    std::lock_guard lock(mutex);
    for (auto& data : render_data)
    {
        if (data.shader_id && std::find(ids.begin(), ids.end(), *data.shader_id) != ids.end())
            data.shader_id = std::nullopt;
        if (data.geometry_shader_id && std::find(ids.begin(), ids.end(), *data.geometry_shader_id) != ids.end())
            data.geometry_shader_id = std::nullopt;
    }
}

void RenderDataManager::remove(RenderDataManagerId id)
{
    std::lock_guard lock(mutex);
    render_data.erase(std::remove_if(render_data.begin(), render_data.end(), [&](RenderData const& data)
    {
        return data.id == id;
    }),
        render_data.end());
}

std::vector<RenderData> const& RenderDataManager::get()
{
    std::lock_guard lock(mutex);
    copy_for_renderer.clear();
    if (render_data.capacity() > copy_for_renderer.capacity())
        copy_for_renderer.reserve(render_data.capacity());
    std::ranges::copy(render_data,
        std::back_inserter(copy_for_renderer));
    return copy_for_renderer;
}

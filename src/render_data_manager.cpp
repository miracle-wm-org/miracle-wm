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

namespace
{
RenderData* find_by_id(std::vector<RenderData>& render_data, RenderDataManagerId id)
{
    auto const it = std::lower_bound(render_data.begin(), render_data.end(), id,
        [](RenderData const& data, RenderDataManagerId id)
    {
        return data.id < id;
    });
    if (it != render_data.end() && it->id == id)
        return &*it;
    return nullptr;
}
}

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
    ++generation;
    return id;
}

void RenderDataManager::transform_change(RenderDataManagerId id, glm::mat4 const& transform)
{
    std::lock_guard lock(mutex);
    if (auto* const data = find_by_id(render_data, id))
    {
        data->transform = transform;
        ++generation;
    }
}

void RenderDataManager::workspace_transform_change(RenderDataManagerId id, glm::mat4 const& transform)
{
    std::lock_guard lock(mutex);
    if (auto* const data = find_by_id(render_data, id))
    {
        data->workspace_transform = transform;
        ++generation;
    }
}

void RenderDataManager::output_area_change(RenderDataManagerId id, mir::geometry::Rectangle const& area)
{
    std::lock_guard lock(mutex);
    if (auto* const data = find_by_id(render_data, id))
    {
        data->output_area = area;
        ++generation;
    }
}

void RenderDataManager::focus_change(RenderDataManagerId id, bool is_focused)
{
    std::lock_guard lock(mutex);
    if (auto* const data = find_by_id(render_data, id))
    {
        data->is_focused = is_focused;
        ++generation;
    }
}

void RenderDataManager::needs_outline_change(RenderDataManagerId id, bool needs_outline)
{
    std::lock_guard lock(mutex);
    if (auto* const data = find_by_id(render_data, id))
    {
        data->needs_outline = needs_outline;
        ++generation;
    }
}

void RenderDataManager::shader_id_change(RenderDataManagerId id, std::optional<uint8_t> shader_id)
{
    std::lock_guard lock(mutex);
    if (auto* const data = find_by_id(render_data, id))
    {
        data->shader_id = shader_id;
        ++generation;
    }
}

void RenderDataManager::reset_shaders(std::vector<uint8_t> const& ids)
{
    std::lock_guard lock(mutex);
    for (auto& data : render_data)
    {
        if (data.shader_id && std::find(ids.begin(), ids.end(), *data.shader_id) != ids.end())
        {
            data.shader_id = std::nullopt;
            ++generation;
        }
    }
}

void RenderDataManager::remove(RenderDataManagerId id)
{
    std::lock_guard lock(mutex);
    auto const it = std::remove_if(render_data.begin(), render_data.end(), [&](RenderData const& data)
    {
        return data.id == id;
    });
    if (it != render_data.end())
    {
        render_data.erase(it, render_data.end());
        ++generation;
    }
}

void RenderDataManager::copy_if_changed(uint64_t& seen_generation, std::vector<RenderData>& out)
{
    std::lock_guard lock(mutex);
    if (seen_generation == generation)
        return;

    seen_generation = generation;
    out = render_data;
}

/**
Copyright (C) 2024  Matthew Kosarek

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